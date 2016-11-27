#pragma once

#include <boost/algorithm/string/join.hpp>

#include <ostream>
#include <tuple>
#include <vector>

template <typename T>
void write(std::ostream& os, T const& v) { os << v; }

template <typename T>
void write(std::ostream& os, std::vector<T> const& v) 
{  os << '[' << boost::algorithm::join(v, ", ") << ']'; }
 
template<class Tuple, std::size_t N>
struct TuplePrinter 
{
    static void print(std::ostream& os, Tuple const& t) 
    {
        TuplePrinter<Tuple, N-1>::print(os, t);
        os << ", "; ::write( os, std::get<N-1>(t));
    }
};

template<class Tuple>
struct TuplePrinter<Tuple, 1> 
{
    static void print(std::ostream& os, Tuple const& t) 
    {  ::write(os, std::get<0>(t)); }
};

struct Writer
{
   template <typename V>
   static void write(std::ostream& os, V const& v)
   {  ::write(os, v); }

   template <typename... V>
   static auto write(std::ostream& os, std::tuple<V...> const& v)// -> typename std::enable_if<(sizeof...(V) > 1)>::type
   {  os << "("; TuplePrinter<decltype(v), sizeof...(V)>::print(os, v); os << ")"; }
   
   //template <typename... V>
   //static auto write(std::ostream& os, std::tuple<V...> const& v) -> typename std::enable_if<(sizeof...(V) == 1)>::type
   //{  ::write(os, std::get<0>(v)); }
};
