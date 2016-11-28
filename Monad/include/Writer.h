#pragma once

#include <boost/algorithm/string/join.hpp>

#include <ostream>
#include <tuple>
#include <vector>
#include <experimental/optional>

struct Writer
{
   template <typename V>
   static void write(std::ostream& os, V const& v)
   {  os << v; }
   
   template <typename V>
   static void write(std::ostream& os, std::experimental::optional<V> const& v)
   {  
      if (v)
      {  write(os, *v); }
      else
      {  os << "unset"; }
   }

   template <typename T>
   static void write(std::ostream& os, std::vector<T> const& v)
   {  os << '[' << boost::algorithm::join(v, ", ") << ']'; }

   template <typename... V>
   static auto write(std::ostream& os, std::tuple<V...> const& v);
};
 
namespace detail
{
   template<class Tuple, std::size_t N>
   struct TupleWriter 
   {
       static void print(std::ostream& os, Tuple const& t) 
       {
           TupleWriter<Tuple, N-1>::print(os, t);
           os << ", "; 
           Writer::write( os, std::get<N-1>(t));
       }
   };

   template<class Tuple>
   struct TupleWriter<Tuple, 1> 
   {
       static void print(std::ostream& os, Tuple const& t) 
       {  Writer::write(os, std::get<0>(t)); }
   };
}

template <typename... V>
auto Writer::write(std::ostream& os, std::tuple<V...> const& v)
{  os << "("; detail::TupleWriter<decltype(v), sizeof...(V)>::print(os, v); os << ")"; }
