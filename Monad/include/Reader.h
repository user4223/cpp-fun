#pragma once

#include <boost/optional/optional.hpp>

#include <istream>
#include <tuple>

struct Reader
{   
   template <typename T, typename V>
   static auto read(std::istream& is, V&& v)
   {  
      T value; 
      is >> value; 
      return createResult(v, value); 
   }
   
   template <typename V>
   static auto readLine(std::istream& is, V&& v)
   {  
      std::string line; 
      std::getline(is, line); 
      return createResult(v, line); 
   }
   
private:
   template <typename T, typename V>
   static auto createResult(V&& v, T&& value)
   {  return std::make_tuple(v, value); }
   
   template <typename T, typename... V>
   static auto createResult(std::tuple<V...> v, T&& value)
   {  return std::tuple_cat(v, std::make_tuple(value)); }
  
// \todo We do need this overload for optionals
// 
//   template <typename T, typename V>
//   static auto createResult(boost::optional<V> v, T&& value)
//   {
//      /** Here we have different return types :-?
//       */
//      if (v)
//      {  return createResult(*v, value); }
//      return std::make_tuple(value);
//   }
};
