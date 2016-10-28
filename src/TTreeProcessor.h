
#include <tuple>
#include <string>
#include <vector>

// We will need std::apply, which isn't available until C++17.
#include "Backports.h"
// Various internal meta-programming helpers
#include "Helpers.h"

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
class TTreeProcessorMapper {
  public:
    virtual T map (InputArgs...) const = 0;

    virtual void finalize() {}

    typedef T output_type;
};

template<typename T, typename... InputArgs>
class TTreeProcessorMapperLambda final : public TTreeProcessorMapper<typename std::result_of<T(InputArgs...)>::type, InputArgs...> {
  public:
    TTreeProcessorMapperLambda(const T& fn) : m_fn(fn) {}

    virtual typename std::result_of<T(InputArgs...)>::type map (InputArgs ...args) const override {
      return m_fn(args...);
    }

  private:
    T m_fn;
};


class InvalidProcessor : public std::exception {
  public:
    virtual const char *what() const noexcept override {return "Attempting to execute an invalid processor handle";}
};


template<typename BranchTypes, typename ... ProcessingStages>
class TTreeProcessor {

    typedef typename internal::convert_to_strings<BranchTypes>::type branch_spec_tuple;
    typedef typename internal::ProcessorResult<BranchTypes, ProcessingStages...>::output_type end_type;

  public:
    /**
     * Construct a processor from a list of branch specifications (may include
     * wildcards)
     *
     * At runtime, we will check that the specified branches actually match the
     * templated class parameters.
     */
    TTreeProcessor(const branch_spec_tuple & branches, ProcessingStages... state) : m_branches(branches), m_stage_state(std::make_tuple(state...))
    {
    }

    TTreeProcessor(const branch_spec_tuple &branches, std::tuple<ProcessingStages&&...> state) : m_branches(branches), m_stage_state(state)
    {}

    TTreeProcessor(TTreeProcessor &&) = default;

    /**
     * Processor object is not copyable.
     */
    TTreeProcessor(TTreeProcessor const&) = delete;
    TTreeProcessor& operator=(TTreeProcessor const&) = delete;

    /**
     * Add a mapping stage to the processor.  The argument must be a lambda that:
     * - Calling function takes the output from previous stage as input
     * - Returns a std::tuple.
     */
    template<typename T> // Hm - it's not clear if we can enforce any of the above with type traits?
    TTreeProcessor<BranchTypes, ProcessingStages..., typename internal::generate_lambda_mapper_type<T, end_type>::type> &&
    map(const T& fn) {

      m_valid = false;
      //typename internal::generate_lambda_mapper_type<T, end_type>::type myMapper(fn);
      //auto states = std::tuple_cat(m_stage_state, std::make_tuple(myMapper));
      return std::move(TTreeProcessor<BranchTypes, ProcessingStages..., typename internal::generate_lambda_mapper_type<T, end_type>::type>
          (m_branches,
           std::tuple_cat(m_stage_state, std::forward_as_tuple( typename internal::generate_lambda_mapper_type<T, end_type>::type (fn)  ))
          ));
    }

    /**
     * Actually perform the processing.
     * TODO: Eventually take a TTree object.  For now, we'll just loop over various numbers.
     */
    void process() {
      if (!m_valid) {throw InvalidProcessor();}
      auto initial_data = std::make_tuple(1, 2, 3);

      for (int i=0; i<10; i++)
      process_stages_helper(initial_data);

      //TODO: call finalize in the end...
    }

  private:

    static const unsigned int stage_count = sizeof...(ProcessingStages);

    template <unsigned int N, unsigned int M, typename Processor>
    struct ProcessorHelper {
      ProcessorHelper(Processor *p_) : m_p(p_) {}
      Processor *m_p;

      void operator()(typename internal::ProcessorArgHelper<0, N, BranchTypes, ProcessingStages...>::input_type arg_tuple) {
        typedef std::decay_t<typename std::tuple_element<N, std::tuple<ProcessingStages...>>::type> stage_type;
        (ProcessorHelper<N+1, M, Processor>(m_p))( internal::std_future::apply(&stage_type::map, std::tuple_cat(std::make_tuple(std::get<N>(m_p->m_stage_state)), arg_tuple)) );
      }
    };

    template <unsigned int N, typename Processor>
    struct ProcessorHelper<N, N, Processor> {
      ProcessorHelper(Processor *p_) : m_p(p_) {}
      Processor *m_p;

      void operator()(typename internal::ProcessorArgHelper<0, N, BranchTypes, ProcessingStages...>::input_type arg_tuple) __attribute__((always_inline)) {
        typedef std::decay_t<typename std::tuple_element<N, std::tuple<ProcessingStages...>>::type> stage_type;
        internal::std_future::apply(&stage_type::map, std::tuple_cat(std::make_tuple(std::get<N>(m_p->m_stage_state)), arg_tuple));
      }
    };

    void
    process_stages_helper(BranchTypes args) {
      ProcessorHelper<0, stage_count-1, typename std::decay<decltype(*this)>::type>(this)(args);
    };

    bool m_valid{true};
    branch_spec_tuple m_branches;
    std::tuple<ProcessingStages...> m_stage_state;
};

}

