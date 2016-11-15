#ifndef __VC_HELPERS_H_
#define __VC_HELPERS_H_

#include <Vc/Vc>

#include "Helpers.h"

namespace ROOT {

// Vector type definitions - 
constexpr size_t vector_count = Vc::float_v::size();
using maskv = Vc::float_v::mask_type;
using floatv = Vc::float_v;
using doublev = Vc::SimdArray<double, vector_count>;
using intv = Vc::SimdArray<int, vector_count>;
using uintv = Vc::SimdArray<unsigned, vector_count>;

namespace internal {

// Conversion templates to go from scalar types to vectored types.
template<typename T>
struct vector_type_impl {
  typedef std::array<T, vector_count> type;
};

template<>
struct vector_type_impl<float> {
  typedef floatv type;
};

template<>
struct vector_type_impl<double> {
  typedef doublev type;
};

template<>
struct vector_type_impl<int> {
  typedef intv type;
};

template<>
struct vector_type_impl<unsigned> {
  typedef uintv type;
};

///
// With this convenience type, a user can determine the type
// of an equivalent vector type by doing:
//
//   template<T>
//   foo( vector_t<T> arg1 ); 
template<typename T>
using vector_t = typename vector_type_impl<T>::type;

///
// Simple SFINAE to determine whether a given function can be applied against
// the vector equivalent of the given arguments.

template<unsigned int I, unsigned int J, unsigned int TypeCode, typename F, typename Tuple, typename... Args>
class is_vectorized_helper {
  public:
    static const bool value = is_vectorized_helper<I+1, J, TypeCode, F, Tuple, Args..., vector_t<typename std::tuple_element<I, Tuple>::type>>::value;
};


///
// Vectorized Helper base case for a mapper
template<unsigned int I, typename F, typename Tuple, typename... Args>
class is_vectorized_helper<I, I, 1, F, Tuple, Args...> {
  private:
    typedef char yes[1];
    typedef char no[2];

    template <typename F2, typename... Args2>
    static yes & test(typename std::result_of<decltype(&F2::map)(F2, maskv, Args2...)>::type *);

    template <typename, typename...>
    static no  & test(...);

  public:
    static const bool value = sizeof(test<F, Args...>(0)) == sizeof(yes);
};

///
// Vectorized Helper base case for a reducer
template<unsigned int I, typename F, typename Tuple, typename... Args>
class is_vectorized_helper<I, I, 0, F, Tuple, Args...> {
  private:
    typedef char yes[1];
    typedef char no[2];

    template <typename F2, typename... Args2>
    static yes & test(typename std::result_of<decltype(&F2::filter)(F2, maskv, Args2...)>::type *);

    template <typename, typename...>
    static no  & test(...);

  public:
    static const bool value = sizeof(test<F, Args...>(0)) == sizeof(yes);
};

template<typename F, typename ArgTuple>
class is_vectorized
{
  public:
    static const bool value = is_vectorized_helper<0, std::tuple_size<ArgTuple>::value, std::is_base_of<TTreeMapper, F>::value, F, ArgTuple>::value;
};

///
// Determine the vectorized version of an input tuple

template<unsigned int I, unsigned int J, typename ArgTuple, typename... VArgs>
class vectorized_tuple_helper {
  public:
    typedef typename vectorized_tuple_helper<I+1, J, ArgTuple, VArgs..., vector_t<typename std::tuple_element<I, ArgTuple>::type>>::type type;
};

template<unsigned int I, typename ArgTuple, typename... VArgs>
class vectorized_tuple_helper<I, I, ArgTuple, VArgs...> {
  public:
    typedef std::tuple<maskv, VArgs...> type;
};

template<typename ArgTuple>
using vectorized_tuple_t = typename vectorized_tuple_helper<0, std::tuple_size<ArgTuple>::value, ArgTuple>::type;

///
// Given a processing chain, determine the input type for the first argument.

template<unsigned int I, unsigned int J, typename ArgTuple, typename... Stages>
class input_tuple_helper;

template<unsigned int I, typename ArgTuple, typename NextStage, typename... Stages>
class input_tuple_helper<0, I, ArgTuple, NextStage, Stages...> {
  public:
    typedef typename std::conditional<is_vectorized<NextStage, ArgTuple>::value, vectorized_tuple_t<ArgTuple>, ArgTuple>::type type;
};

template<typename ArgTuple>
class input_tuple_helper<0, 0, ArgTuple> {
  public:
    typedef ArgTuple type;
};

template<typename ArgTuple, typename... Stages>
using input_tuple_t = typename input_tuple_helper<0, sizeof...(Stages), ArgTuple, Stages...>::type;

///
// Given a processing chain, determine if it is vectorized.
//
template<unsigned int I, unsigned int J, typename ArgTuple, typename... Stages>
class is_vectorized_stream_helper;

template<unsigned int I, typename ArgTuple, typename NextStage, typename... Stages>
class is_vectorized_stream_helper<0, I, ArgTuple, NextStage, Stages...> {
  public:
    static const bool value = is_vectorized<NextStage, ArgTuple>::value;
};

template<typename ArgTuple>
class is_vectorized_stream_helper<0, 0, ArgTuple> {
  public:
    static const bool value = false;
};

template<typename ArgTuple, typename... Stages>
class is_vectorized_stream {
  public:
    static const bool value = is_vectorized_stream_helper<0, sizeof...(Stages), ArgTuple, Stages...>::value;
};


}  // internal

}  // ROOT

#endif  // __VC_HELPERS_H_
