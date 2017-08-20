#include <iostream>
#include <string>
#include <typeinfo>

#include <boost/hana.hpp>

struct Foo { BOOST_HANA_DEFINE_STRUCT(Foo,
   (int, x),
   (int, y),
   (double, z)
);};

//BOOST_HANA_ADAPT_STRUCT(Foo, x, y, z);

struct Bar { BOOST_HANA_DEFINE_STRUCT(Bar,
   (float, value),
   (std::string, description)
);};

//BOOST_HANA_ADAPT_STRUCT(Bar, p, s);

template<typename TypeT>
void printStruct(TypeT const& instance)
{
   boost::hana::for_each(instance, [](auto pair) {
   std::cout << "type: " << typeid(boost::hana::second(pair)).name() << ", "
             << "name: " << boost::hana::to<char const*>(boost::hana::first(pair)) << ", "
             << "value: " << boost::hana::second(pair) << std::endl;
   });
}
