#pragma once

#include <tuple>
#include <experimental/optional>

namespace stde = std::experimental;

namespace detail
{
   /** Non-tuples and tuples with more than 1 entry
       are forwarded as they are.
    */
   template <typename V>
   static auto normalize(V&& v) 
   {  return std::forward<V>(v); }

   /** Tuples with exactly 1 entry are transformed
       to the plain value.
    */
   template <typename V>
   static auto normalize(std::tuple<V>&& v) -> V 
   {  return std::get<0>(v); }
}

/** Argument is moved into function and return value
    of the function is moved further. Function transforms
    the given value.
 */
template <typename T, typename F>
auto operator | (T&& t, F&& f) -> typename std::enable_if<!std::is_void<decltype(f(t))>::value, decltype(detail::normalize(f(t)))>::type 
{  return detail::normalize(f(std::forward<T>(t))); }

/** Function is called with a copy of the argument.
    The argument is forwarded to the next.
    \remark We don't have to normalize here, because the argument is forwarded and not changed, hence it is already normalized
 */
template <typename T, typename F>
auto operator | (T&& t, F&& f) -> typename std::enable_if<std::is_void<decltype(f(t))>::value, T>::type
{  
   f(t); 
   return std::forward<T>(t); //< forward parameter
} 

/** Function is called when the optional is present with a copy of the value.
    The optional argument is forwarded to the next.
    \remark We don't have to normalize here, because the argument is forwarded and not changed, hence it is already normalized
 */
template <typename T, typename F>
auto operator | (stde::optional<T>&& t, F&& f) -> typename std::enable_if<std::is_void<decltype(f(t))>::value, stde::optional<T>>::type
{  
   if (t) 
   {  f(*t); } 
   return std::forward<stde::optional<T>>(t); //< forward parameter
}

/** When the argument is optional, the function is
    called only when the argument is set.
 */
template <typename T, typename F>
auto operator | (stde::optional<T>&& t, F&& f) -> stde::optional<decltype(detail::normalize(f(*t)))>
{  return t ? detail::normalize(f(*t)) : stde::optional<decltype(detail::normalize(f(*t)))>(); }
