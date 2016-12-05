#pragma once

#include <experimental/optional>

template <typename Q>
struct OptionalAware
{
   /** The the argument is different from optional,
    *  just call the specific function.
    */
   template <typename T>
   static auto call(T&& v) { return Q::call(v); }
   
   /** This is never called because pipe operator
    *  has an overload for optional and handles it 
    *  differently there. But the definition has to 
    *  be there because the compiler generates code 
    *  for the case regardless.  
    */
   template <typename T>
   static auto call(std::experimental::optional<T> v) { assert(false); }
};
