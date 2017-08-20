
//#include "../include/TypeChecksum.h"

#include <boost/type_traits.hpp>
#include <boost/mpl/range_c.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/preprocessor.hpp>
#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include <boost/functional/hash.hpp>

#include <gtest/gtest.h>

#include <typeinfo>
#include <iostream>

#include <cxxabi.h>

#define REM(...) __VA_ARGS__
#define EAT(...)
#define STRIP(x) EAT x        // Strip off the type
#define PAIR(x) REM x         // Show the type without parenthesis

namespace detail
{
   // A helper metafunction for adding const to a type
   template<class M, class T>
   struct make_const { typedef T type; };

   template<class M, class T>
   struct make_const<const M, T> { typedef typename boost::add_const<T>::type type; };
}

#define REFLECTABLE(...) \
static const int memberCount = BOOST_PP_VARIADIC_SIZE(__VA_ARGS__); \
public: bool hasMemberInfo() const { return true; } private: \
friend struct detail::reflector; \
template<int N, class ClassType> struct MemberInfo {}; \
BOOST_PP_SEQ_FOR_EACH_I(REFLECT_EACH, data, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#define REFLECT_EACH(r, data, index, member) \
PAIR(member); \
template<class ClassType> \
struct MemberInfo<index, ClassType> \
{ \
   ClassType const& m_class; \
   MemberInfo(ClassType const& _class) : m_class(_class) {} \
   typename detail::make_const<ClassType, decltype(member)>::type& get() \
   {  return m_class.STRIP(member); } \
   typename boost::add_const<decltype(member)>::type& get() const \
   {  return m_class.STRIP(member); } \
   std::string type() const \
   { \
      int status(0); \
      auto name(typeid(member).name()); \
      auto result(abi::__cxa_demangle(name, 0, 0, &status)); \
      return status == 0 ? result : name; \
   } \
   std::string name() const \
   {  return BOOST_PP_STRINGIZE(STRIP(member)); } \
}; \

namespace detail 
{
   template<class T> class supportsMemberInfo
   {
       typedef char (&Yes)[1];
       typedef char (&No)[2];

       template<class U> 
       static Yes check(U const* i, typename std::enable_if<std::is_same<bool, decltype(i->hasMemberInfo())>::value>::type* = 0);
       template<class U> 
       static No check(...);
   public:
       static const bool value = sizeof(Yes) == sizeof(supportsMemberInfo::check<T>((typename std::remove_reference<T>::type*)0));
   };

   struct reflector
   {
      template<int N, class ClassType> ///< Get memberInfo at index N
      static typename ClassType::template MemberInfo<N, ClassType> getMemberInfo(ClassType const& _class)
      {  return typename ClassType::template MemberInfo<N, ClassType>(_class); }
         
      template<class ClassType>        ///< Get the number of fields
      struct members
      {  static const int count = ClassType::memberCount; };
   };

   struct memberVisitor
   {
      template<class ClassType, class Visitor, class MemberType>
      void operator()(ClassType const& c, Visitor visitor, MemberType const&);
   };
   
   template<class ClassType, class Visitor>
   void visitMember(ClassType const& c, Visitor visitor)
   {
      typedef boost::mpl::range_c<int, 0, reflector::members<ClassType>::count> range;
      boost::mpl::for_each<range>(boost::bind<void>(memberVisitor(), boost::ref(c), visitor, _1));
   }
   
   template<class ClassType, class Visitor, class MemberType>
   void memberVisitor::operator()(ClassType const& c, Visitor visitor, MemberType const& m)
   {
      visitor(reflector::getMemberInfo<MemberType::value>(c));
      
      /** \todo Add recursion into member when 
                detail::supportsMemberInfo<MemberType::value>::value is true
                
          if supportsMemberInfo -> visitMember(m, visitor);
      */   
   }
} ///< detail

template<class ClassType>
std::vector<std::tuple<std::string, std::string>> getMemberList(ClassType const& c)
{  
   std::vector<std::tuple<std::string, std::string>> memberList;
   detail::visitMember(c, [&](auto const& memberInfo){ memberList.emplace_back(std::make_tuple(memberInfo.type(), memberInfo.name())); }); 
   return memberList;
}

template<class ClassType>
size_t getTypeHash(ClassType const& c)
{
   auto const memberList(getMemberList(c));
   size_t seed(0);
   for (auto const& item : memberList) ///< Currently we use the types only to calculate the hash value
   {  boost::hash_combine(seed, std::get<0>(item)); }
   return seed;
}

///< User code

struct Address
{
   Address(std::string road, unsigned int no) : m_road(std::move(road)), m_no(no) {}

   std::string toString() const
   {
      std::ostringstream os;
      os << "road: " << m_road << ", no: " << m_no;
      return os.str();
   }

private:
   REFLECTABLE
   (
      (std::string) m_road,
      (unsigned int) m_no
   )
};

struct Dimensions 
{
   Dimensions(int height) : m_height(height) {}
   
   std::string toString() const 
   {
      std::ostringstream os;
      os << "height: " << m_height;
      return os.str();
   }
   
private:
   int m_height;
};

struct Person
{
    Person(const char *name, int age, Address address, Dimensions dimensions) : 
       m_name(name)
      ,m_age(age)
      ,m_address(std::move(address))
      ,m_dimensions(std::move(dimensions)) 
    {}
    
    std::string toString() const
    {
       std::ostringstream os;
       os << "name: " << m_name << ", "
          << "age: " << m_age << ", "
          << "address: (" << m_address.toString() << "), "
          << "dimensions: (" << m_dimensions.toString() << ")";
       return os.str();
    }

private:
    REFLECTABLE
    (
        (const char *) m_name,
        (int) m_age,
        (Address) m_address,
        (Dimensions) m_dimensions
    )
};

TEST(TypeCheck, Simple)
{
   Person majorTom("Major Tom", 42, Address("Spaceroad", 23), Dimensions(179));
   std::cout << "content: ("  << majorTom.toString() << ")\n" 
             << "type hash: " << getTypeHash(majorTom) << '\n';
             
   Person eT("E.T.", 300, Address("Spaceroad", 5), Dimensions(142));
   std::cout << "content: ("  << eT.toString() << ")\n" 
             << "type hash: " << getTypeHash(eT) << '\n';
   
   std::cout << detail::supportsMemberInfo<Person>::value      << '\n'; ///< yes
   std::cout << detail::supportsMemberInfo<Address>::value     << '\n'; ///< yes
   std::cout << detail::supportsMemberInfo<Dimensions>::value  << '\n'; ///< no
   std::cout << detail::supportsMemberInfo<int>::value         << '\n'; ///< no
   
   {
      auto memberList(getMemberList(majorTom));
      for (auto const& member : memberList)
      {  std::cout << std::get<0>(member) << ": " << std::get<1>(member) << '\n'; }
   }
   {
      auto memberList(getMemberList(eT));
      for (auto const& member : memberList)
      {  std::cout << std::get<0>(member) << ": " << std::get<1>(member) << '\n'; }
   }   
}
