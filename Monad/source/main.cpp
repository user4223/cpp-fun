
#include "../include/Pipe.h"
#include "../include/Reader.h"
#include "../include/Writer.h"
#include "../include/OptionalAware.h"

#include <boost/algorithm/string.hpp>

#include <iostream>
#include <tuple>
#include <vector>
#include <experimental/optional>

struct Multiplier : public OptionalAware<Multiplier>
{   
   template <typename A, typename B> static auto call(std::tuple<A, B> v)
   {  return std::get<0>(v) * std::get<1>(v); }
};

struct Devider : public OptionalAware<Devider>
{
   template <typename A, typename B> static auto call(std::tuple<A, B> v)
   {
      auto b(std::get<1>(v));
      if (b == 0)
      {  return std::experimental::optional<decltype(std::get<0>(v) / b)>( /*empty*/ ); }
      return std::experimental::make_optional(std::get<0>(v) / b);
   }
};

struct Counter : public OptionalAware<Counter>
{
   template <typename T> static auto call(T&& v) 
   {  
      auto c(v.size());
      //if (c == 0)
      //{  return std::experimental::optional<int>( /*empty*/ ); }
      //return std::experimental::make_optional(static_cast<int>(c)); 
      return static_cast<int>(c);
   }
};

struct WordSplitter : public OptionalAware<WordSplitter>
{
   static auto call(std::string v) ///< Possible for strings only? Or vectors and lists as well?
   {
      std::vector<std::string> words;
      boost::split(words, v, boost::is_any_of("\t "));
      return std::make_tuple(words);
   }
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
   
   auto const multiply  ([](auto v) { return OptionalAware<Multiplier  >::call(v); });
   auto const devide    ([](auto v) { return OptionalAware<Devider     >::call(v); });
   auto const count     ([](auto v) { return OptionalAware<Counter     >::call(v); });
   auto const splitWords([](auto v) { return OptionalAware<WordSplitter>::call(v); });
   
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
