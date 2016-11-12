
#include <tuple>
#include <type_traits>

/**
 * Several of the template meta-programming techniques I'm
 * interested in are planned for C++17: the ones below are
 * those that can be backported (library-only; needs no
 * language features, of course).
 */

namespace ROOT {

namespace internal {

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

/** Slight twist on std::apply; makes it easier to do class methods */
namespace detail {
template <class F, class F_obj, class Tuple, std::size_t... I>
constexpr decltype(auto) apply_method_impl( F&& f, F_obj& f_obj, Tuple&& t, std::index_sequence<I...> )
{
  return invoke(std::forward<F>(f), f_obj, std::get<I>(std::forward<Tuple>(t))...);
}
} // namespace detail

template <class F, class F_obj, class Tuple>
constexpr decltype(auto) apply_method(F&& f, F_obj& f_obj, Tuple&& t)
{
    return detail::apply_method_impl(std::forward<F>(f), f_obj, std::forward<Tuple>(t),
        std::make_index_sequence<tuple_size_v<std::decay_t<Tuple>>>{});
}

}  // namespace std_future

}  // namespace internal

}  // namespace ROOT

