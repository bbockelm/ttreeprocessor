
#include <tuple>
#include <string>
#include <vector>
#include <numeric>

#include "tbb/task_group.h"
//#include "tbb/combinable.h"
#include "tbb/enumerable_thread_specific.h"

#include "TFile.h"
#include "TTreeReader.h"
#include "ROOT/TThreadedObject.hxx"

// We will need std::apply, which isn't available until C++17.
#include "Backports.h"
// Various internal meta-programming helpers
#include "Helpers.h"
#include "RootHelpers.h"

namespace ROOT {

/**
 * A class for efficiently processing events in a TTree.
 *
 * The TTreeProcessor is designed to provide a multithreaded, MapReduce-style
 * processing interfaceb for iterating through events in a TTree.
 *
 * The goal is to replace the traditional b for-loop for ROOT:
 *
 * TTree *tree = static_cast<TTree*>(file->GetObject("MyTree"));
 * // Set up a bunch of branches
 * for (Int_t idx=0l; idx<tree->GetEntries(); idx++) {
 *   tree->GetEntry(idx); // 
 *   momentum = x*x + y*y + z*z; // Map X, Y, Z 
 *   if (momentum < 20) {continue;} // Cut based on momentum.
 *   hist->Fill(njets); // Fill a histogram
 * }
 *
 * With something like this:
 *
 * TH1 hist; // Initialize...
 * auto processor = TTreeProcessor<Float_t, Float_t, Float_t, Int_t>({"x", "y", "z", "njets"})
 *    .map([](Float_t x, Float_t y, Float_z, Int_t njets) {
 *       return std::make_tuple(x*x + y*y + z*z, njets);
 *     })
 *    .filter([](Float_t momentum, Int_t njets) { return momentum < 20; })
 *    .apply([&](Float_t momentum, Int_t njets) { hist->Fill(njets); });
 *
 * processor.process(file->GetObject("MyTree"));
 *
 * The goal is that the above expressions "look like" the stream processing ideas
 * from Java (note: these are all trivially parallelizable), but:
 * - The input always comes from a TTree.
 * - Eventually allow vectorizable constructs (replace `Float_t x` with Vc-like `Float_8t x` to process 8 events at a time).
 * - Heavily utilize metaprogramming so the compiler can inline all function calls.
 * The last point is important: rather than depend solely on polymorphism, we aim to construct
 * the framework such that all the lambdas are part of the same function body.
 */


/**
 * The base class of mappers.
 *
 * Note that `map` is invoked as const; that means any access
 * to a member variable must be THREAD SAFE!
 */
template<typename T, typename... InputArgs>
class TTreeProcessorMapper : public TTreeMapper {
  public:
    TTreeProcessorMapper() {}
    TTreeProcessorMapper(const TTreeProcessorMapper&) = delete;
    TTreeProcessorMapper(TTreeProcessorMapper&&) = default;

    T map (InputArgs...) const noexcept {};

    bool finalize() {return true;}

    typedef T output_type;
};


/**
 * Definition of a mapper derived from a user-provided lambda function.
 */
template<typename T, typename... InputArgs>
class TTreeProcessorMapperLambda final : public TTreeProcessorMapper<typename std::result_of<T(InputArgs...)>::type, InputArgs...> {
  public:
    TTreeProcessorMapperLambda(const T& fn) : m_fn(fn) {}

    typename std::result_of<T(InputArgs...)>::type map (InputArgs ...args) const noexcept {
      return m_fn(args...);
    }

  private:
    T m_fn;
};

void __attribute__((noinline)) foo() {std::cout << "foo\n";}

/**
 * Counts the number of events seen by a mapper.
 * Prints the final number out at the end.
 */
template<typename... InputArgs>
class TTreeProcessorCountPrinter final : public TTreeProcessorMapper<std::tuple<InputArgs...>, InputArgs...> {
  typedef tbb::enumerable_thread_specific<int> EventCounter;

  public:
    TTreeProcessorCountPrinter() : m_counter(0) {
      std::cout << "Creating new instance of Counter.\n";
    }

    TTreeProcessorCountPrinter(TTreeProcessorCountPrinter && rhs) {
      std::cout << "Moving a counter.\n";
      foo();
      m_counter = std::move(rhs.m_counter);
    }

    TTreeProcessorCountPrinter(const TTreeProcessorCountPrinter & rhs) {
      std::cout << "Copying a counter.\n";
      int sum = std::accumulate(rhs.m_counter.begin(), rhs.m_counter.end(), 0);
      m_counter = EventCounter(sum);
    }

    std::tuple<InputArgs...> map (InputArgs... args) const noexcept {
      //EventCounter::reference my_counter = m_counter->local();
      //++m_counter->local();
      m_counter.local()++;

      return std::make_tuple(args...);
    }

    bool finalize() {
      int sum = std::accumulate(m_counter.begin(), m_counter.end(), 0);
      //int sum = m_counter->combine([](int x, int y) {return x+y;});
      std::cout << "Counter saw " << sum << " events.\n";
    }

  private:
    mutable EventCounter m_counter;
};

/**
 * Base class of a filter.
 */
template<typename... InputArgs>
class TTreeProcessorFilter : public TTreeFilter {
  public:
    TTreeProcessorFilter() {}
    TTreeProcessorFilter(const TTreeProcessorFilter&) = delete;
    TTreeProcessorFilter(TTreeProcessorFilter&&) = default;

    bool filter(InputArgs...) const noexcept {};

    bool finalize() {return true;}
};

/**
 * Filters derived from a user-provided lambda.
 */
template<typename T, typename... InputArgs>
class TTreeProcessorFilterLambda final : public TTreeProcessorFilter<InputArgs...> {
  public:
    TTreeProcessorFilterLambda(const T& fn) : m_fn(fn) {}

    bool filter(InputArgs ...args) const noexcept {
      return m_fn(args...);
    }

  private:
    T m_fn;
};


class InvalidProcessor : public std::exception {
  public:
    virtual const char *what() const noexcept override {return "Attempting to execute an invalid processor handle";}
};


class NoSuchTree : public std::exception {

  public:
    NoSuchTree(const std::string & treename, TFile *tf) {
      std::stringstream ss;
      ss << "No tree named " << treename << " in file " << tf->GetEndpointUrl()->GetUrl();
      m_msg = ss.str();
    }

    virtual const char *what() const noexcept override {return m_msg.c_str();}

  private:
    std::string m_msg;
};


template<typename BranchTypes, typename ... ProcessingStages>
class TTreeProcessor {

    typedef typename internal::convert_to_strings<BranchTypes>::type branch_spec_tuple;
    typedef typename internal::ProcessorResult<BranchTypes, ProcessingStages...>::output_type end_type;
    template<class T> using stage_initializer_t = typename std::conditional<std::is_move_constructible<T>::value, T&&, T&>::type;
    template<class T> using stage_storage_t = typename std::conditional<std::is_move_constructible<T>::value, T, T&>::type;

  public:
    /**
     * Construct a processor from a list of branch specifications (may include
     * wildcards)
     *
     * At runtime, we will check that the specified branches actually match the
     * templated class parameters.
     */

    explicit
    TTreeProcessor(const branch_spec_tuple & branches, internal::stage_initializer_t<ProcessingStages>... state) : m_branches(branches), m_stage_state(std::forward_as_tuple(std::move(state)...))
    {
        ROOT::EnableThreadSafety();
    }

    TTreeProcessor(const branch_spec_tuple &branches, std::tuple<internal::stage_initializer_t<ProcessingStages>...>&& state) : m_branches(branches), m_stage_state(std::move(state))
    {
        ROOT::EnableThreadSafety();
    }

    /**
     * Processor object is not copyable or moveable.
     */
    TTreeProcessor(TTreeProcessor &&) = delete;
    TTreeProcessor(TTreeProcessor const&) = delete;
    TTreeProcessor& operator=(TTreeProcessor const&) = delete;

    /**
     * Add a mapping stage to the processor.  The argument must be a lambda that:
     * - Calling function takes the output from previous stage as input
     * - Returns a std::tuple.
     */
    template<typename T> // Hm - it's not clear if we can enforce any of the above with type traits?
    TTreeProcessor<BranchTypes, ProcessingStages..., typename internal::generate_lambda_type<TTreeProcessorMapperLambda, T, end_type>::type> &&
    map(const T& fn) {

      m_valid = false;
      /*
      return std::move(TTreeProcessor<BranchTypes, ProcessingStages..., typename internal::generate_lambda_type<TTreeProcessorMapperLambda, T, end_type>::type>
          (m_branches,
           std::tuple_cat(m_stage_state, std::forward_as_tuple( typename internal::generate_lambda_type<TTreeProcessorMapperLambda, T, end_type>::type (fn)  ))
          ));
      */
      return internal::construct_processor<TTreeProcessor<BranchTypes, ProcessingStages..., typename internal::generate_lambda_type<TTreeProcessorMapperLambda, T, end_type>::type>, decltype(m_branches), decltype(m_stage_state), typename internal::generate_lambda_type<TTreeProcessorMapperLambda, T, end_type>::type>
      (
          m_branches,
          m_stage_state,
          typename internal::generate_lambda_type<TTreeProcessorMapperLambda, T, end_type>::type(fn)
      );
    }

    /**
     * Add a filter stage to the processor.  The argument must be a lambda that
     * - Takes the output from the previous stage as input.
     * - Returns a bool.
     * If the lambda returns false, the current event is ignored for the rest of the chain.
     */
    template<typename T>  // TODO: enforce calling signature via type_traits
    TTreeProcessor<BranchTypes, ProcessingStages..., typename internal::generate_lambda_type<TTreeProcessorFilterLambda, T, end_type>::type> &&
    filter(const T& fn) {
      m_valid = false;
      /*return std::move(TTreeProcessor<BranchTypes, ProcessingStages..., typename internal::generate_lambda_type<TTreeProcessorFilterLambda, T, end_type>::type>
          (m_branches,
           std::tuple_cat(m_stage_state, std::forward_as_tuple( typename internal::generate_lambda_type<TTreeProcessorFilterLambda, T, end_type>::type(fn) ))
          ));
      */
      return internal::construct_processor<TTreeProcessor<BranchTypes, ProcessingStages..., typename internal::generate_lambda_type<TTreeProcessorFilterLambda, T, end_type>::type>, decltype(m_branches), decltype(m_stage_state), typename internal::generate_lambda_type<TTreeProcessorFilterLambda, T, end_type>::type>
      (
          m_branches,
          m_stage_state,
          typename internal::generate_lambda_type<TTreeProcessorFilterLambda, T, end_type>::type(fn)
      );
    }

    /**
     * Add a verbose counter - prints out how many events passed the map function.
     */
    TTreeProcessor<BranchTypes, ProcessingStages..., TTreeProcessorCountPrinter<end_type>> &&
    count() {
      m_valid = false;
      /*
      return std::move(TTreeProcessor<BranchTypes, ProcessingStages..., TTreeProcessorCountPrinter<end_type>>
          (m_branches,
           std::tuple_cat(m_stage_state, std::forward_as_tuple( TTreeProcessorCountPrinter<end_type>() ))
          ));
      */
      return internal::construct_processor<TTreeProcessor<BranchTypes, ProcessingStages..., TTreeProcessorCountPrinter<end_type>>, decltype(m_branches), decltype(m_stage_state), TTreeProcessorCountPrinter<end_type>>
      (
          m_branches,
          m_stage_state,
          TTreeProcessorCountPrinter<end_type>()
      );
    }

    /**
     * Process a set of TTrees in a list of files.
     * 
     */
    void process(const std::string &treeName, std::vector<TFile*> inputFiles) {
      if (!m_valid) {throw InvalidProcessor();}

      for (auto tf : inputFiles) {
          TTreeReader myReader(treeName.c_str(), tf);
          auto readerValues = internal::make_reader_tuple<BranchTypes>(myReader, m_branches);
          while (myReader.Next()) {
              BranchTypes event_data = internal::read_event_data<BranchTypes>(readerValues);
              process_stages_helper(event_data);
          }
      }
      finalize();
    }

    void processParallel(const std::string &treeName, std::vector<TFile*> inputFiles) {
      if (!m_valid) {throw InvalidProcessor();}

      tbb::task_group g;
      std::vector<std::shared_ptr<ROOT::TThreadedObject<internal::TFileHelper>>> scope_helper;  // Keep TThreadedObject in-scope.
      scope_helper.reserve(inputFiles.size());
      for (auto tf : inputFiles) {
          scope_helper.emplace_back(std::make_shared<ROOT::TThreadedObject<internal::TFileHelper>>(tf->GetEndpointUrl()->GetUrl()));
          ROOT::TThreadedObject<internal::TFileHelper> &ts_file = *(scope_helper.back());
          Long64_t clusterStart;
          TTree *tree = static_cast<TTree*>(tf->GetObjectChecked(treeName.c_str(), "TTree"));
          if (!tree) {
              throw NoSuchTree(treeName, tf);
          }
          TTree::TClusterIterator clusterIter = tree->GetClusterIterator(0);
          while ( (clusterStart = clusterIter()) < tree->GetEntries() ) {
              Long64_t clusterEnd = clusterIter.GetNextEntry();
              g.run([&, clusterStart, clusterEnd]() {
                  // TODO: Would make a lot of sense to reuse the reader/value objects via TThreadedObject.
                  TFile *tf = (ts_file.Get())->get();
                  if (!tf) {
                    std::cerr << "Failed to get thread-safe TFile object.\n";
                    return;
                  }
                  TTreeReader myReader(treeName.c_str(), tf);
                  auto readerValues = internal::make_reader_tuple<BranchTypes>(myReader, m_branches);
                  myReader.SetEntriesRange(clusterStart, clusterEnd);

                  while (myReader.Next()) {
                      BranchTypes event_data = internal::read_event_data<BranchTypes>(readerValues);
                      process_stages_helper(event_data);
                  }
              });
          }
      }
      g.wait();
      finalize();
    }

  private:

    static const unsigned int stage_count = sizeof...(ProcessingStages);

    /**
     * ProcesorHelper assists in applying each consecutive stage in the chain.
     *
     * Template arguments:
     * - N: Current offset in the processing stages.
     * - M: Maximum number of stages to process.
     * - IsMapper: Set to 1 if this is a mapper, 0 otherwise.
     * - Processor: Base type of the processor.
     */
    template <unsigned int N, unsigned int M, unsigned int IsMapper, typename Processor>
    struct ProcessorHelper;

    // Recursion case for a mapper.
    template <unsigned int N, unsigned int M, typename Processor>
    struct ProcessorHelper<N, M, 1, Processor> {
      ProcessorHelper(Processor *p_) : m_p(p_) {}
      Processor *m_p;

      static const unsigned int next_is_mapper = internal::GetStageType<N+1, ProcessingStages...>::value;

      void operator()(typename internal::ProcessorArgHelper<0, N, BranchTypes, ProcessingStages...>::input_type arg_tuple) {
        typedef std::decay_t<typename std::tuple_element<N, std::tuple<ProcessingStages...>>::type> stage_type;
        (ProcessorHelper<N+1, M, next_is_mapper, Processor>(m_p))( internal::std_future::apply_method(&stage_type::map, std::get<N>(m_p->m_stage_state), arg_tuple));
      }
    };

    // Recursion case for a filter.
    template <unsigned int N, unsigned int M, typename Processor>
    struct ProcessorHelper<N, M, 0, Processor> {
      ProcessorHelper(Processor *p_) : m_p(p_) {}
      Processor *m_p;

      static const unsigned int next_is_mapper = internal::GetStageType<N+1, ProcessingStages...>::value;

      void operator()(typename internal::ProcessorArgHelper<0, N, BranchTypes, ProcessingStages...>::input_type arg_tuple) {
        typedef std::decay_t<typename std::tuple_element<N, std::tuple<ProcessingStages...>>::type> stage_type;
        bool result = internal::std_future::apply_method(&stage_type::filter, std::get<N>(m_p->m_stage_state), arg_tuple);
        if (!result) {return;}  // Event did not pass the filter; stop processing.
        (ProcessorHelper<N+1, M, next_is_mapper, Processor>(m_p))( arg_tuple ); // Pass input argument directly to the next stage.
      }
    };

    // Base case for a mapper.
    template <unsigned int N, typename Processor>
    struct ProcessorHelper<N, N, 1, Processor> {
      ProcessorHelper(Processor *p_) : m_p(p_) {}
      Processor *m_p;

      void operator()(typename internal::ProcessorArgHelper<0, N, BranchTypes, ProcessingStages...>::input_type arg_tuple) __attribute__((always_inline)) {
        typedef std::decay_t<typename std::tuple_element<N, std::tuple<ProcessingStages...>>::type> stage_type;
        internal::std_future::apply_method(&stage_type::map, std::get<N>(m_p->m_stage_state), arg_tuple);
      }
    };

    // Base case for a filter.
    // Doesn't seem to make much sense to run a filter at the end - maybe will be more useful in the future if we have counters
    // of events that pass.
    template <unsigned int N, typename Processor>
    struct ProcessorHelper<N, N, 0, Processor> {
      ProcessorHelper(Processor *p_) : m_p(p_) {}
      Processor *m_p;

      void operator()(typename internal::ProcessorArgHelper<0, N, BranchTypes, ProcessingStages...>::input_type arg_tuple) __attribute__((always_inline)) {
        typedef std::decay_t<typename std::tuple_element<N, std::tuple<ProcessingStages...>>::type> stage_type;
        internal::std_future::apply_method(&stage_type::filter, std::get<N>(m_p->m_stage_state), arg_tuple);
      }
    };

    void
    process_stages_helper(BranchTypes args) {
      ProcessorHelper<0, stage_count-1, internal::GetStageType<0, ProcessingStages...>::value, typename std::decay<decltype(*this)>::type>(this)(args);
    };

    // Invoke all the finalize methods.
    template <std::size_t ...I>
    void finalize_helper (std::index_sequence<I...>) {
        std::make_tuple(std::get<I>(m_stage_state).finalize() ...);
    }

    void
    finalize() {
        finalize_helper( std::make_index_sequence< sizeof...(ProcessingStages) >() );
    }

    bool m_valid{true};
    branch_spec_tuple m_branches;

    // If the type is move constructible, perform the move.
    // Otherwise, take a reference.
    std::tuple< stage_storage_t<ProcessingStages>...> m_stage_state;
};

}

