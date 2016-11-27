#pragma once

#include <istream>
#include <tuple>

struct Reader
{   
   template <typename T, typename V>
   static auto read(std::istream& is, V&& v)
   {  T value; is >> value; return std::make_tuple(v, value); }

   template <typename T, typename... V>
   static auto read(std::istream& is, std::tuple<V...> v)
   {  T value; is >> value; return std::tuple_cat(v, std::make_tuple(value)); }
   
   template <typename V>
   static auto readLine(std::istream& is, V&& v)
   {  std::string line; std::getline(is, line); return std::make_tuple(v, line); }

   template <typename... V>
   static auto readLine(std::istream& is, std::tuple<V...> v)
   {  std::string line; std::getline(is, line); return std::tuple_cat(v, std::make_tuple(line)); }
};
