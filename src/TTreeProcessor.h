
#include <tuple>
#include <string>
#include <vector>

namespace ROOT {

namespace internal {

/**
 * For each branch type specification (say, std::tuple<float, int, double>),
 * the user needs to provide a set of corresponding branch names.  This set of
 * templates will generate an appropriate typedef of std::tuple<std::string, ...>
 * given a particular input tuple.
 */
template<unsigned int I, unsigned int J, typename... Args>
class convert_to_strings_helper {
  public:
    typedef typename convert_to_strings_helper<I+1, J, std::string, Args...>::type type;
};

template<unsigned int I, typename... Args>
class convert_to_strings_helper<I, I, Args...> {
  public:
    typedef std::tuple<Args...> type;
};

template<typename... Args>
class convert_to_strings_helper<1, 0, Args...> {
  public:
    typedef std::tuple<> type;
};


template<class BaseClass>
class convert_to_strings {
  private:
    static const int string_count = std::tuple_size<BaseClass>::value;

  public:
    typedef typename convert_to_strings_helper<1, string_count, std::string>::type type;
};

namespace std_future {

/** Backports of helper types from C++17 */
template< class T >
constexpr bool is_function_v = std::is_function<T>::value;
template< class Base, class Derived >
constexpr bool is_base_of_v = std::is_base_of<Base, Derived>::value;
template< class T >
constexpr bool is_member_pointer_v = std::is_member_pointer<T>::value;
template< class T >
constexpr std::size_t tuple_size_v = std::tuple_size<T>::value;

/** Sample implementation of std::invoke until C++17 is available */
namespace detail {
template <class T>
struct is_reference_wrapper : std::false_type {};
template <class U>
struct is_reference_wrapper<std::reference_wrapper<U>> : std::true_type {};
template <class T>
constexpr bool is_reference_wrapper_v = is_reference_wrapper<T>::value;
 
template <class Base, class T, class Derived, class... Args>
auto INVOKE(T Base::*pmf, Derived&& ref, Args&&... args)
    noexcept(noexcept((std::forward<Derived>(ref).*pmf)(std::forward<Args>(args)...)))
 -> std::enable_if_t<is_function_v<T> &&
                     is_base_of_v<Base, std::decay_t<Derived>>,
    decltype((std::forward<Derived>(ref).*pmf)(std::forward<Args>(args)...))>
{
      return (std::forward<Derived>(ref).*pmf)(std::forward<Args>(args)...);
}
 
template <class Base, class T, class RefWrap, class... Args>
auto INVOKE(T Base::*pmf, RefWrap&& ref, Args&&... args)
    noexcept(noexcept((ref.get().*pmf)(std::forward<Args>(args)...)))
 -> std::enable_if_t<is_function_v<T> &&
                     is_reference_wrapper_v<std::decay_t<RefWrap>>,
    decltype((ref.get().*pmf)(std::forward<Args>(args)...))>
 
{
      return (ref.get().*pmf)(std::forward<Args>(args)...);
}
 
template <class Base, class T, class Pointer, class... Args>
auto INVOKE(T Base::*pmf, Pointer&& ptr, Args&&... args)
    noexcept(noexcept(((*std::forward<Pointer>(ptr)).*pmf)(std::forward<Args>(args)...)))
 -> std::enable_if_t<is_function_v<T> &&
                     !is_reference_wrapper_v<std::decay_t<Pointer>> &&
                     !is_base_of_v<Base, std::decay_t<Pointer>>,
    decltype(((*std::forward<Pointer>(ptr)).*pmf)(std::forward<Args>(args)...))>
{
      return ((*std::forward<Pointer>(ptr)).*pmf)(std::forward<Args>(args)...);
}
 
template <class Base, class T, class Derived>
auto INVOKE(T Base::*pmd, Derived&& ref)
    noexcept(noexcept(std::forward<Derived>(ref).*pmd))
 -> std::enable_if_t<!is_function_v<T> &&
                     is_base_of_v<Base, std::decay_t<Derived>>,
    decltype(std::forward<Derived>(ref).*pmd)>
{
      return std::forward<Derived>(ref).*pmd;
}
 
template <class Base, class T, class RefWrap>
auto INVOKE(T Base::*pmd, RefWrap&& ref)
    noexcept(noexcept(ref.get().*pmd))
 -> std::enable_if_t<!is_function_v<T> &&
                     is_reference_wrapper_v<std::decay_t<RefWrap>>,
    decltype(ref.get().*pmd)>
{
      return ref.get().*pmd;
}
 
template <class Base, class T, class Pointer>
auto INVOKE(T Base::*pmd, Pointer&& ptr)
    noexcept(noexcept((*std::forward<Pointer>(ptr)).*pmd))
 -> std::enable_if_t<!is_function_v<T> &&
                     !is_reference_wrapper_v<std::decay_t<Pointer>> &&
                     !is_base_of_v<Base, std::decay_t<Pointer>>,
    decltype((*std::forward<Pointer>(ptr)).*pmd)>
{
      return (*std::forward<Pointer>(ptr)).*pmd;
}
 
template <class F, class... Args>
auto INVOKE(F&& f, Args&&... args)
    noexcept(noexcept(std::forward<F>(f)(std::forward<Args>(args)...)))
 -> std::enable_if_t<!is_member_pointer_v<std::decay_t<F>>,
    decltype(std::forward<F>(f)(std::forward<Args>(args)...))>
{
      return std::forward<F>(f)(std::forward<Args>(args)...);
}
} // namespace detail
 
template< class F, class... ArgTypes >
auto invoke(F&& f, ArgTypes&&... args)
    // exception specification for QoI
    noexcept(noexcept(detail::INVOKE(std::forward<F>(f), std::forward<ArgTypes>(args)...)))
 -> decltype(detail::INVOKE(std::forward<F>(f), std::forward<ArgTypes>(args)...))
{
    return detail::INVOKE(std::forward<F>(f), std::forward<ArgTypes>(args)...);
}

/** Sample implementation of std::apply until C++17 is available */
namespace detail {
template <class F, class Tuple, std::size_t... I>
constexpr decltype(auto) apply_impl( F&& f, Tuple&& t, std::index_sequence<I...> )
{
  return invoke(std::forward<F>(f), std::get<I>(std::forward<Tuple>(t))...);
}
} // namespace detail
 
template <class F, class Tuple>
constexpr decltype(auto) apply(F&& f, Tuple&& t)
{
    return detail::apply_impl(std::forward<F>(f), std::forward<Tuple>(t),
        std::make_index_sequence<tuple_size_v<std::decay_t<Tuple>>>{});
}

} // namespace std_future

}

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

    typename std::result_of<T(InputArgs...)>::type operator() (InputArgs... args) const { return fn(args...); }

  private:
    T m_fn;
};

namespace internal {
/**
 * The lambda-based Mapper will be given a std::tuple<Args...> for the input type; however,
 * we need to unpack the Args directly into the template specification.  This set of classes
 * generate the appropriate type.
 *
 * For example, if we are given a lambda function of type T and know the input is
 * std::tuple<int, double, double>, then we want to generate the type:
 *   TTreeProcessorMapperLambda<T, int, double, double>
 *
 */

template<unsigned int I, unsigned int J, typename T, typename InputTuple, typename... Args>
struct generate_lambda_helper {
    typedef typename generate_lambda_helper<I+1, J, T, InputTuple, Args..., typename std::tuple_element<I, InputTuple>::type>::type type;
};

template<unsigned int I, typename T, typename InputTuple, typename... Args>
struct generate_lambda_helper<I, I, T, InputTuple, Args...> {
    typedef TTreeProcessorMapperLambda<T, Args...> type;
};

template<typename T, class InputTuple>
class generate_lambda_mapper_type {
  private:
    static const int type_count = std::tuple_size<InputTuple>::value;

  public:
    typedef typename generate_lambda_helper<0, type_count, T, InputTuple>::type type;
};

}

/**
 * The base class of end-mappers.
 *
 * These are fed the current products of the stream, yet do not
 * return any further values.
 *
 * As with mappers, the `map` function is invoked as const -- it
 * must be thread-safe and will indeed be called from multiple threads.
 *
 * However, the end-mapper additionally has `finalize` called; this is
 * done from a single-threaded context after all events have been processed.
 *
 * For example, if there is a thread-local histogram, we would fill it with
 * each input during `map` and then reduce all the thread-locals into a global
 * histogram during `finalize`.
 */
/*
template<std::tuple<InputArgs...>>
class TTreeProcessorEndMapper {
  public:
    virtual void map const (InputArgs...) = 0;
    virtual void finalize () = 0;
};

template<std::tuple<Args..>, ProcessingStages...>
class TTreeProcessorHelper {
  public:
    void apply(std::add_lvalue_reference<Args>::type ...);
};
*/

/**
 * Given a set of branch types and processing stages, calculate the
 * input / output arguments.
 */
template<unsigned int I, unsigned int J, typename F, typename InputTuple, typename... Args>
struct result_of_unpacked_tuple;

template<unsigned int I, unsigned int J, typename F, typename InputTuple, typename... Args>
struct result_of_unpacked_tuple {
  typedef typename result_of_unpacked_tuple<I+1, J, F, InputTuple, Args..., typename std::tuple_element<I, InputTuple>::type>::type type;
};

template<unsigned int I, typename F, typename InputTuple, typename... Args>
struct result_of_unpacked_tuple <I, I, F, InputTuple, Args...> {
  /* NOTE: this likely needs to be cleaned up.  We really want to call F.map(Args..) instead. */
  typedef typename std::result_of<F(Args...)>::type type;
};

template<unsigned int I, unsigned int J, typename InputArg, typename... ProcessingStages>
struct ProcessorArgHelper;

template<unsigned int I, unsigned int J, typename InputArg, typename F, typename... ProcessingStages>
struct ProcessorArgHelper<I,J, InputArg, F, ProcessingStages...> {
  typedef typename ProcessorArgHelper<I, J-1, InputArg, F, ProcessingStages...>::output_type input_type;
  typedef typename ProcessorArgHelper<I+1, J, input_type, F, ProcessingStages...>::output_type output_type;
};

template<unsigned int I, typename InputArg, typename F, typename... Args>
struct ProcessorArgHelper<I, I, InputArg, F, Args...> {
  typedef InputArg input_type;
  typedef typename result_of_unpacked_tuple<0, std::tuple_size<input_type>::value, F, input_type>::type output_type;
};

template<typename InputArg, typename... ProcessingStages>
struct ProcessorResult {
  typedef typename ProcessorArgHelper<0, sizeof...(ProcessingStages)-1, InputArg, ProcessingStages...>::output_type output_type;
};

template<typename InputArg>
struct ProcessorResult<InputArg> {
  typedef InputArg output_type;
};

class InvalidProcessor : public std::exception {
  public:
    virtual const char *what() const noexcept override {return "Attempting to execute an invalid processor handle";}
};

template<typename BranchTypes, typename ... ProcessingStages>
class TTreeProcessor {

    typedef typename internal::convert_to_strings<BranchTypes>::type branch_spec_tuple;
    typedef typename ProcessorResult<BranchTypes, ProcessingStages...>::output_type end_type;

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

      process_stages_helper(initial_data);

      //TODO: call finalize in the end...
    }

  private:

    static const unsigned int stage_count = sizeof...(ProcessingStages);

    template <unsigned int N, unsigned int M, typename Processor>
    struct ProcessArgHelper {
    };

    template <unsigned int N, unsigned int M, typename Processor>
    struct ProcessorHelper {
      ProcessorHelper(Processor *p_) : m_p(p_) {}
      Processor *m_p;

      typename ProcessorArgHelper<0, N, BranchTypes, ProcessingStages...>::output_type operator()(typename ProcessorArgHelper<0, N, BranchTypes, ProcessingStages...>::input_type arg_tuple) {
        return ProcessorHelper<N+1, M, Processor>(this)(internal::std_future::apply(std::get<N>(m_p->m_stage_state), arg_tuple));
      }
    };

    template <unsigned int N, typename Processor>
    struct ProcessorHelper<N, N, Processor> {
      ProcessorHelper(Processor *p_) : m_p(p_) {}
      Processor *m_p;

      void operator()(typename ProcessorArgHelper<0, N, BranchTypes, ProcessingStages...>::input_type arg_tuple) {}
    };

    void
    process_stages_helper(BranchTypes args) {
      ProcessorHelper<0, stage_count-1, typename std::decay<decltype(*this)>::type>(this)(args);
    };

    bool m_valid{true};
    branch_spec_tuple m_branches;
    std::tuple<ProcessingStages...> m_stage_state;
};
/*
template<typename BranchTypes, typename ... ProcessingStages, typename Mapper>
class TTreeProcessor<BranchTypes, ProcessingStages..., Mapper> {
  public:
    TTreeProcessor(TTreeProcessor<BranchTypes, ProcessingStages...> &previous, Mapper next) {
      previous.m_valid = false;
      m_branches = std::move(previous.m_branches);
      m_stage_state = std::tuple_cat(std::forward_as_tuple(previous.m_stage_state), next);
    }
};
*/

}

