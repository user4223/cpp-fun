
#include "../include/Pipe.h"
#include "../include/Reader.h"
#include "../include/Writer.h"

#include <boost/algorithm/string.hpp>

#include <iostream>
#include <tuple>
#include <vector>
#include <experimental/optional>

struct multiplier
{
   template <typename A, typename B>
   static auto multiply(std::tuple<A, B> v) 
   {  return std::get<0>(v) * std::get<1>(v); }
   
   template <typename T>
   static auto multiply(stde::optional<T> v) 
   {  
      if (v)
      {  return stde::make_optional(multiply(*v)); }
      return stde::optional<decltype(multiply(*v))>();
   }
};

struct devider
{
   template <typename A, typename B>
   static auto devide(std::tuple<A, B> v)
   {
      auto b(std::get<1>(v));
      if ( b == 0)
      {  return stde::optional<double>(); }
      return stde::make_optional(std::get<0>(v) / b);
   }
   
   template <typename T>
   static auto devide(stde::optional<T> v) 
   {  
      if (v)
      {  return stde::make_optional(devide(*v)); }
      return stde::optional<decltype(devide(*v))>();  
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
   
   auto const multiply  ([](auto v) { return multiplier::multiply(v); });
   auto const devide    ([](auto v) { return devider::devide(v); });
   auto const count     ([](auto v) { return static_cast<int>(v.size()); });
   
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
