
#include <string>
#include <tuple>

/**
 * Various helper functions related to template meta-programming.
 *
 * We often need to generate various types for variadic arguments
 * (such as converting std::tuple<Args...> to a Args... parameter
 * pack as part of a template).  These are the routines we'll use.
 */

namespace ROOT {

class TTreeMapper {};
class TTreeFilter {};

namespace internal {

/**
 *  GENERATE A TYPENAME FOR THE TTreeProcessor*Lambda templates.
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

template<unsigned int I, unsigned int J, template<typename T, typename... Args> class LambdaClass, typename T, typename InputTuple, typename... Args>
struct generate_lambda_helper {
    typedef typename generate_lambda_helper<I+1, J, LambdaClass, T, InputTuple, Args..., typename std::tuple_element<I, InputTuple>::type>::type type;
};

template<unsigned int I, template<typename T, typename... Args> class LambdaClass, typename T, typename InputTuple, typename... Args>
struct generate_lambda_helper<I, I, LambdaClass, T, InputTuple, Args...> {
    typedef LambdaClass<T, Args...> type;
};

template<template<typename T, typename... Args> class LambdaClass, typename T, class InputTuple>
class generate_lambda_type {
  private:
    static const int type_count = std::tuple_size<InputTuple>::value;

  public:
    typedef typename generate_lambda_helper<0, type_count, LambdaClass, T, InputTuple>::type type;
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
 * Given a set of branch types and a list of processing stages, calculate the
 * input / output arguments.
 *
 * This requires us to examine each function F(Args...) -> std::tuple<Args2...> and
 * apply them in a chain.
 *
 * Notes:
 * - result_of_unpacked_tuple* takes the output tuple (std::tuple<Args2...>) and a class F,
 *   then calculates the result of F::map(Args2...).
 * - ProcessorApply will look at the base type of F and determine whether or not it is a
 *   mapper; if it is not, then it assumes the types in the chain are unchanged.
 * - ProcessorArgHelper is the user-visible interface: you give it a list of processing stages
 *   and the offset in the stage, and it will calculate the input and output arguments.
 */

template<unsigned int I, unsigned int J, typename F, typename InputTuple, typename... Args>
struct result_of_unpacked_tuple_helper;

template<unsigned int I, unsigned int J, typename F, typename InputTuple, typename... Args>
struct result_of_unpacked_tuple_helper {
  typedef typename result_of_unpacked_tuple_helper<I+1, J, F, InputTuple, Args..., typename std::tuple_element<I, InputTuple>::type>::type type;
};

template<unsigned int I, typename F, typename InputTuple, typename... Args>
struct result_of_unpacked_tuple_helper <I, I, F, InputTuple, Args...> {
  typedef typename std::result_of<decltype(&F::map)(F, Args...)>::type type;
};

template<typename F, typename InputTuple>
struct result_of_unpacked_tuple {
  typedef typename result_of_unpacked_tuple_helper<0, std::tuple_size<InputTuple>::value, F, InputTuple>::type type;
};

/**
 * ProcessorApply and ProcessorApplyHelper take a class type (F):
 *
 * - If F derives from TTreeMapper, then apply the unpacked tuple to the
 *   class's map function.
 * - Otherwise, assume that the class descends from TTreeFilter and assume.
 *   the stream's types are unchanged (as the filter simply removes events,
 *   not changes types).
 */
template<unsigned int I, typename F, typename InputArg>
struct ProcessorApplyHelper;

template<typename F, typename InputArg>
struct ProcessorApplyHelper<0, F, InputArg> {
  typedef InputArg type;
};

template<typename F, typename InputArg>
struct ProcessorApplyHelper<1, F, InputArg> {
  typedef typename result_of_unpacked_tuple<F, InputArg>::type type;
};

template<typename F, typename InputArg>
struct ProcessorApply {
  static const unsigned int is_mapper = std::is_base_of<TTreeMapper, F>::value;
  typedef typename ProcessorApplyHelper<is_mapper, F, InputArg>::type type;
};

template<unsigned int I, unsigned int J, typename InputArg, typename... ProcessingStages>
struct ProcessorArgHelper;

template<unsigned int I, unsigned int J, typename InputArg, typename F, typename... ProcessingStages>
struct ProcessorArgHelper<I,J, InputArg, F, ProcessingStages...> {
  // This is the input tuple for the Jth function in ProcessingStages.
  typedef typename ProcessorArgHelper<0, J-1, InputArg, F, ProcessingStages...>::output_type input_type;
  // This is the next input type for ProcessingStages.
  typedef typename ProcessorArgHelper<0, 0, InputArg, F, ProcessingStages...>::output_type next_input_arg;
  // This is the output tuple for the Jth function in ProcessingStages.
  typedef typename ProcessorArgHelper<I+1, J, next_input_arg, ProcessingStages...>::output_type output_type;
};

template<unsigned int I, typename InputArg, typename F, typename... Args>
struct ProcessorArgHelper<I, I, InputArg, F, Args...> {
  typedef InputArg input_type;
  typedef typename ProcessorApply<F, input_type>::type output_type;
};

template<unsigned int I, typename InputArg>
struct ProcessorArgHelper<I, I, InputArg> {
  typedef InputArg input_type;
  typedef InputArg output_type;
};

/**
 * ProcessorResult calculates the final output type after all the processing stages, given a particular
 * input type.
 */
template<typename InputArg, typename... ProcessingStages>
struct ProcessorResult {
  typedef typename ProcessorArgHelper<0, sizeof...(ProcessingStages)-1, InputArg, ProcessingStages...>::output_type output_type;
};

template<typename InputArg>
struct ProcessorResult<InputArg> {
  typedef InputArg output_type;
};

/**
 * Given a list of stages, determine whether stage N is a mapper.
 */

template<unsigned int I, unsigned int J, typename F, typename... ProcessingStages>
struct GetStageTypeHelper {
  static const unsigned int value = GetStageTypeHelper<I+1, J, ProcessingStages...>::value;
};

template<unsigned int N, typename F, typename... ProcessingStages>
struct GetStageTypeHelper<N, N, F, ProcessingStages...> {
  static const unsigned int value = std::is_base_of<TTreeMapper, F>::value;
};

template<unsigned int N, typename... ProcessingStages>
struct GetStageType {
  static const unsigned int value = GetStageTypeHelper<0, N, ProcessingStages...>::value;
};

}  // internal

}  // ROOT

