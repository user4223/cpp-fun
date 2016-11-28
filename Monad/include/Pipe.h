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
   {  return std::get<0>(v); /* cannot be perfect forwarded, is extracted element */ }
   
   template <typename V>
   static auto normalize(stde::optional<std::tuple<V>>&& v) -> stde::optional<V> 
   {
      if (v)
      {  return stde::make_optional(normalize(*v)); }
      return stde::optional<V>();
   }
}

/** Argument is moved into function and return value
    of the function is moved further. Function transforms
    the given value.
   
    Non-void function version.
 */
template <typename T, typename F>
auto operator | (T&& t, F&& f) -> typename std::enable_if<!std::is_void<decltype(detail::normalize(f(t)))>::value, decltype(detail::normalize(f(t)))>::type 
{  return std::forward<decltype(detail::normalize(f(t)))>(detail::normalize(f(std::forward<T>(t)))); }

/** When the argument is optional, the function is
    called only when the argument is set.

    Non-void function version for optional arguments.
 */
template <typename T, typename F>
auto operator | (stde::optional<T>&& t, F&& f) -> typename std::enable_if<!std::is_void<decltype(detail::normalize(f(*t)))>::value, stde::optional<decltype(operator|(*t, f))>>::type
{  
   if (t)
   {  return stde::make_optional(operator|(*t, f)); }
   return stde::optional<decltype(operator|(*t, f))>();
}

/** Function is called with a copy of the argument.
    The argument is forwarded to the next.
    
    \remark We don't have to normalize here, because the argument is forwarded and not changed, hence it is already normalized
   
    Void function version.
 */
template <typename T, typename F>
auto operator | (T&& t, F&& f) -> typename std::enable_if<std::is_void<decltype(f(t))>::value, T>::type
{  
   auto const& _t(t);
   f(_t); ///< Force const reference as parameter
   return std::forward<T>(t); //< forward parameter
} 

/** Function is called when the optional is present with a copy of the value.
    The optional argument is forwarded to the next.
    
    \remark We don't have to normalize here, because the argument is forwarded and not changed, hence it is already normalized
   
    Void function version for optional arguments.
 */
template <typename T, typename F>
auto operator | (stde::optional<T>&& t, F&& f) -> typename std::enable_if<std::is_void<decltype(f(*t))>::value, stde::optional<T>>::type
{  
   if (t) 
   {  return stde::make_optional(operator|(*t, f)); } 
   return std::forward<stde::optional<T>>(t); //< forward parameter
}
