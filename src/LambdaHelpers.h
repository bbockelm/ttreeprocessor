#ifndef __LAMBDA_HELPERS_H_
#define __LAMBDA_HELPERS_H_

#include "VcHelpers.h"

namespace ROOT {

namespace internal {

template<unsigned int I, unsigned int J, unsigned int IsVectorized, template<unsigned int IsVec, typename T, typename... Args> class LambdaClass, typename T, typename InputTuple, typename... Args>
struct generate_lambda_helper_vectorized;

template<unsigned int I, unsigned int J, template<unsigned int IsVectorized, typename T, typename... Args> class LambdaClass, typename T, typename InputTuple, typename... Args>
struct generate_lambda_helper_vectorized<I, J, 0, LambdaClass, T, InputTuple, Args...> {
    typedef typename generate_lambda_helper_vectorized<I+1, J, 0, LambdaClass, T, InputTuple, Args..., typename std::tuple_element<I, InputTuple>::type>::type type;
};

template<unsigned int I, template<unsigned int IsVectorized, typename T, typename... Args> class LambdaClass, typename T, typename InputTuple, typename... Args>
struct generate_lambda_helper_vectorized<I, I, 0, LambdaClass, T, InputTuple, Args...> {
    typedef LambdaClass<0, T, Args...> type;
};

template<unsigned int I, unsigned int J, template<unsigned int IsVectorized, typename T, typename... Args> class LambdaClass, typename T, typename InputTuple, typename... Args>
struct generate_lambda_helper_vectorized<I, J, 1, LambdaClass, T, InputTuple, Args...> {
    typedef typename generate_lambda_helper_vectorized<I+1, J, 1, LambdaClass, T, InputTuple, Args..., internal::vector_t<typename std::tuple_element<I, InputTuple>::type>>::type type;
};

template<unsigned int I, template<unsigned int IsVectorized, typename T, typename... Args> class LambdaClass, typename T, typename InputTuple, typename... Args>
struct generate_lambda_helper_vectorized<I, I, 1, LambdaClass, T, InputTuple, Args...> {
    typedef LambdaClass<1, T, maskv, Args...> type;
};

template<template<unsigned int IsVectorized, typename T, typename... Args> class LambdaClass, typename T, class InputTuple>
class generate_lambda_type_vectorized {
  private:
    static const int type_count = std::tuple_size<InputTuple>::value;
    static const bool is_vectorized = internal::is_vectorized<T, InputTuple>::value;
    //static const bool is_vectorized = 1;

  public:
    typedef typename generate_lambda_helper_vectorized<0, type_count, is_vectorized, LambdaClass, T, InputTuple>::type type;
};

}  // internal

}  // ROOT

#endif
