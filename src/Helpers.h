
/**
 * Various helper functions related to template meta-programming.
 *
 * We often need to generate various types for variadic arguments
 * (such as converting std::tuple<Args...> to a Args... parameter
 * pack as part of a template).  These are the routines we'll use.
 */

namespace ROOT {

// Forward dec'l for generate_lambda_mapper_type below.
template<typename T, typename... InputArgs> class TTreeProcessorMapperLambda;

namespace internal {

/**
 *  GENERATE A TYPENAME FOR THE TTreeProcessorMapperLambda.
 *
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

/**
 * GENERATE A TUPLE OF STRINGS OF A CERTAIN LENGTH
 *
 *
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

}  // internal

}  // ROOT

