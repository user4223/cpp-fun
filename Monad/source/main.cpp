
#include "../include/Pipe.h"
#include "../include/Reader.h"
#include "../include/Writer.h"

#include <boost/algorithm/string.hpp>

#include <iostream>
#include <tuple>
#include <vector>
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
   static auto call(stde::optional<T> v) { assert(false); }
};

struct Multiplier : public OptionalAware<Multiplier>
{   
   template <typename A, typename B> 
   static auto call(std::tuple<A, B> v) { return std::get<0>(v) * std::get<1>(v); }
};

struct Devider : public OptionalAware<Devider>
{
   template <typename A, typename B>
   static auto call(std::tuple<A, B> v)
   {
      auto b(std::get<1>(v));
      if ( b == 0)
      {  return stde::optional<double>(); }
      return stde::make_optional(std::get<0>(v) / b);
   }
};

struct Counter : public OptionalAware<Counter>
{
   template <typename T> 
   static auto call(T&& v) { return static_cast<int>(v.size()); }
};

int main(int argc, char** argv)
{
   auto& os(std::cout);
   auto& is(std::cin);
      
   //auto const readString([&](auto v) { return Reader::read<std::string>(is, v); });
   //auto const readInt   ([&](auto v) { return Reader::read<int>(is, v); });
   auto const readLine  ([&](auto v) { return Reader::readLine(is, v); });
   auto const readReal  ([&](auto v) { return Reader::read<double>(is, v); });
   auto const write     ([&](auto v) { Writer::write(os, v); });
   auto const trace     ([&](auto v) { os << "trace: "; Writer::write(os, v); os << '\n'; });
   
   auto const multiply  ([](auto v) { return OptionalAware<Multiplier>::call(v); });
   auto const devide    ([](auto v) { return OptionalAware<Devider>::call(v); });
   auto const count     ([](auto v) { return OptionalAware<Counter>::call(v); });
   
   auto const splitWords([](auto v) 
   { 
      std::vector<std::string> words;
      boost::split(words, v, boost::is_any_of("\t "));
      return std::make_tuple(words) ;
   });
   
   //stde::optional<std::tuple<>>()
   std::make_tuple()
      | readLine     |  trace
      | splitWords   |  trace
      | count        |  trace
      | readReal     |  trace
      | devide       |  trace
      | readReal     |  trace
      | multiply     |  trace
      | write;
   
   os << '\n';
}
