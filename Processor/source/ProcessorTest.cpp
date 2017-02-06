
#include "../include/Processor.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <list>
#include <vector>
#include <algorithm>
#include <chrono>

/** \todo Add builder to create complex processor setup
    \todo Add chain of responsibility
 */

struct Uncopyable
{
   explicit Uncopyable( int v ) : m_value( v ) {}
   
   Uncopyable( Uncopyable const& ) = delete;
   Uncopyable& operator=( Uncopyable const& ) = delete;
   
   Uncopyable( Uncopyable&& other ) = default;
   Uncopyable& operator=( Uncopyable&& other ) = default;
   
   int m_value;
};

TEST( Queue, PushPop )
{
   Queue< int > queue;
   queue.Push( 23 );
   EXPECT_EQ( 23, queue.Pop() );
}

TEST( Queue, MultiPushPop )
{
   Queue< int > queue;
   queue.Push( 23 );
   queue.Push(  5 );
   queue.Push(  7 );
   EXPECT_EQ( 23, queue.Pop() );
   EXPECT_EQ(  5, queue.Pop() );
   EXPECT_EQ(  7, queue.Pop() );
}

TEST( Queue, EmptyPop )
{
   Queue< int > queue;
   EXPECT_FALSE( queue.Pop() );
   EXPECT_FALSE( queue.Pop() );
}

TEST( Queue, PushPopOrWait )
{
   Queue< int > queue;
   queue.Push( 23 );
   EXPECT_EQ( 23, queue.PopOrWait() );
}

TEST( Queue, MultiPushPopOrWait )
{
   Queue< int > queue;
   queue.Push( 23 );
   queue.Push(  5 );
   queue.Push(  7 );
   EXPECT_EQ( 23, queue.PopOrWait() );
   EXPECT_EQ(  5, queue.PopOrWait() );
   EXPECT_EQ(  7, queue.PopOrWait() );
}

TEST( Queue, EmptyPopOrWait )
{
   Queue< int > queue;
   {
      auto start( std::chrono::system_clock::now() );
      EXPECT_FALSE( queue.PopOrWait( std::chrono::seconds( 1 ) ) );
      /** Takes longer than 1 second */
      EXPECT_TRUE( 1000 <= std::chrono::duration_cast< std::chrono::milliseconds >( std::chrono::system_clock::now() - start ).count() );
   }
   {
      auto start( std::chrono::system_clock::now() );
      auto result( std::async( std::launch::async, [&]
      {  return queue.PopOrWait( std::chrono::seconds( 2 ) ); } ) );
      queue.Push( 23 );
      auto optional( result.get() );
      /** Takes less than 2 second */
      EXPECT_TRUE( 2000 >= std::chrono::duration_cast< std::chrono::milliseconds >( std::chrono::system_clock::now() - start ).count() );
      EXPECT_TRUE( (bool)optional );
      EXPECT_EQ( 23, optional.value() );
   }
}

TEST( Queue, CancelBreaksWait )
{
   auto const start( std::chrono::system_clock::now() );
   Queue< int > queue;
   auto result( std::async( std::launch::async, [&]
   {  return queue.PopOrWait( std::chrono::seconds( 5 ) ); } ) );
   queue.Cancel();
   result.get();
   auto const end( std::chrono::system_clock::now() );
   EXPECT_TRUE( std::chrono::duration_cast< std::chrono::seconds >( end - start ) < std::chrono::seconds( 5 ) );
}

TEST( Queue, CancelStopsPush )
{
   Queue< int > queue;
   queue.Push( 23 );
   queue.Push(  5 );
   queue.Cancel();
   EXPECT_THROW( queue.Push( 7 ), std::logic_error );
   EXPECT_EQ( 23, queue.Pop().value() );
   EXPECT_EQ(  5, queue.Pop().value() );
   EXPECT_FALSE( queue.Pop() );   
}

TEST( Queue, Uncopyable )
{
   Queue< Uncopyable > queue;
   queue.Push( Uncopyable(23) );
   queue.Push( Uncopyable( 5) );
   queue.Cancel();
   EXPECT_THROW( queue.Push( Uncopyable(7) ), std::logic_error );
   EXPECT_EQ( 23, queue.Pop().value().m_value );
   EXPECT_EQ(  5, queue.PopOrWait().value().m_value );
   EXPECT_FALSE( queue.Pop() );   
}

TEST( BufferingTaskProcessor, ConstructDestroy )
{
   BufferingTaskProcessor< int > processor( 2 );
}

TEST( BufferingTaskProcessor, OrderedPushAndPop )
{
   BufferingTaskProcessor< int > processor( 4 );
   for ( int no( 0 ); no < 100; ++no )
   { 
      processor.Push( [=]{ return no; } );
   }
   for ( int no( 0 ); no < 100; ++no )
   {
      auto item( processor.PopOrWait() );
      EXPECT_TRUE( (bool)item );
      EXPECT_EQ( no, item->get() );
   }
}

TEST( BufferingTaskProcessor, Throw )
{
   BufferingTaskProcessor< int > processor( 1 );
   processor.Push( []()-> int { throw std::exception(); } );
   processor.Push( []{ return 3; } );
   processor.Push( []()-> int { throw std::exception(); } );
   processor.Push( []{ return 7; } );
   EXPECT_THROW( processor.PopOrWait()->get(), std::exception );
   EXPECT_EQ( 3, processor.PopOrWait()->get() );
   EXPECT_THROW( processor.PopOrWait()->get(), std::exception );
   EXPECT_EQ( 7, processor.PopOrWait()->get() );
   EXPECT_FALSE( processor.Pop() );
}

TEST( BufferingTaskProcessor, CancelStopsPush )
{
   BufferingTaskProcessor< int > processor( 4 );
   processor.Push( []{ return 23; } );
   processor.Push( []{ return  5; } );
   processor.Cancel();
   EXPECT_THROW( processor.Push( []{ return 7; } ), std::logic_error );
   EXPECT_EQ( 23, processor.Pop()->get() );
   EXPECT_EQ(  5, processor.Pop()->get() );
   EXPECT_FALSE( processor.Pop() );
}

TEST( BufferingTaskProcessor, TerminatingVoid )
{
   std::atomic< int > exception( 0 ), called( 0 );
   BufferingTaskProcessor<> a( 4 );
   TerminationProcessor<> b( a, [ &exception ]( std::future< void > input )
   {
      try { input.get(); }
      catch ( std::exception const& e )
      {  ++exception; }
   } );
   a.Push( [ &called ]{ ++called; } );
   a.Push( []{ throw std::exception(); } );
   a.Push( [ &called ]{ ++called; } );
   a.Cancel();
   b.Wait();
   EXPECT_EQ( 2, called.load() );
   EXPECT_EQ( 1, exception.load() );
}

TEST( TaskProcessor, ConstructDestroy )
{
   TaskProcessor< int > processor( 2 );
}

TEST( TaskProcessor, PushPop )
{
   TaskProcessor< int > processor( 4 );
   std::vector< std::future< int > > futures;
   for ( int no( 0 ); no < 100; ++no )
   { 
      futures.emplace_back( processor.Push( [=]{ return no; } ) );
   }
   EXPECT_TRUE( futures.size() > 0 );
   std::accumulate( futures.begin(), futures.end(), 0, []( int value, std::future< int >& future )
   {
      EXPECT_EQ( value, future.get() );
      return value + 1;
   } );
}

TEST( TaskProcessor, Chaining )
{
   TaskProcessor< int > a( 2 );
   TaskProcessor< int > b( 2 );
   std::vector< std::future< int > > futures;
   for ( int no( 0 ); no < 100; ++no )
   { 
      futures.emplace_back( b.Push(a.Push([=]{ return no; }), [](int v){ return v * 2; }) );
   }
   for ( int no( 0 ); no < 100; ++no )
   {
      EXPECT_EQ(no * 2, futures[no].get());
   }   
}

TEST( TaskProcessor, Throw )
{
   TaskProcessor< int > processor( 1 );
   std::vector< std::future< int > > futures;
   futures.emplace_back( processor.Push( []()-> int { throw std::exception(); } ) );
   futures.emplace_back( processor.Push( []{ return 3; } ) );
   futures.emplace_back( processor.Push( []()-> int { throw std::exception(); } ) );
   futures.emplace_back( processor.Push( []{ return 7; } ) );
   EXPECT_THROW( futures[0].get(), std::exception );
   EXPECT_EQ( 3, futures[1].get() );
   EXPECT_THROW( futures[2].get(), std::exception );
   EXPECT_EQ( 7, futures[3].get() );
}

TEST( TaskProcessor, CancelStopsPush )
{
   TaskProcessor< int > processor( 4 );
   auto futureA( processor.Push( []{ return 23; } ) );
   auto futureB( processor.Push( []{ return  5; } ) );
   processor.Cancel();
   EXPECT_THROW( processor.Push( []{ return 7; } ), std::logic_error );
   EXPECT_EQ(  5, futureB.get() );
   EXPECT_EQ( 23, futureA.get() );
}

TEST( TaskProcessor, Void )
{
   TaskProcessor<> processor( 4 );
   std::atomic< int > called( 0 );
   auto futureA( processor.Push( [ &called ]{ ++called; } ) );
   auto futureB( processor.Push( [ &called ]{ ++called; } ) );
   processor.Cancel();
   EXPECT_THROW( processor.Push( [ &called ]{ ++called; } ), std::logic_error );
   EXPECT_NO_THROW( futureB.get() );
   EXPECT_NO_THROW( futureA.get() );
   EXPECT_EQ( 2, called.load() );
}

TEST( TaskProcessor, Uncopyable )
{
   TaskProcessor< Uncopyable > processor( 4 );
   auto futureA( processor.Push( []{ return Uncopyable( 23 ); } ) );
   auto futureB( processor.Push( []{ return Uncopyable(  5 ); } ) );
   processor.Cancel();
   EXPECT_THROW( processor.Push( []{ return Uncopyable(  7 ); } ), std::logic_error );
   EXPECT_EQ( 23, futureA.get().m_value );
   EXPECT_EQ(  5, futureB.get().m_value );
}

TEST( ContinuationBufferingTaskProcessor, PushPop )
{
   BufferingTaskProcessor< int > a( 4 );
   ContinuationBufferingTaskProcessor<int, float> b( 4, a );
   ContinuationBufferingTaskProcessor<float, std::array<unsigned char, 3>> c( 4, b );
   
   std::vector< std::future< int > > futures;
   for ( int no( 0 ); no < 100; ++no )
   { 
      a.Push([no]       { return no; });
      b.Push([](int v)  { return v * 2.f; });
      c.Push([](float v)
      {  
         auto c((unsigned char)v);
         return std::array<unsigned char, 3>({c, c, c}); 
      });
   }
   for ( unsigned char no( 0 ); no < 200; no += 2 )
   {
      std::array<unsigned char, 3> v({no, no, no});
      EXPECT_EQ( v, c.PopOrWait()->get() );
   }   
}

TEST( DataProcessor, ConstructDestroy )
{
   DataProcessor< int, int > processor( 1, []( int i ){ return i; } );
}

TEST( DataProcessor, PushPop )
{
   DataProcessor< int, int > processor( 1, []( int i ) 
   { 
      return i * 2; 
   } );
   processor.Push( 23 );
   processor.Push( 5 );
   EXPECT_EQ( 46, processor.PopOrWait()->get() );
   EXPECT_EQ( 10, processor.PopOrWait()->get() );
}

TEST( DataProcessor, Throw )
{
   DataProcessor< int, int > processor( 1, []( int i ) 
   { 
      if ( i == 5 )
      {  throw std::exception(); }
      return i * 2; 
   } );
   processor.Push( 23 );
   processor.Push(  5 );
   processor.Push(  7 );
   EXPECT_EQ( 46, processor.PopOrWait()->get() );
   EXPECT_THROW( processor.PopOrWait()->get(), std::exception );
   EXPECT_EQ( 14, processor.PopOrWait()->get() );
}

TEST( DataProcessor, UnequalInputOutputTypes )
{
   DataProcessor< int, double > processor( 1, []( int i ) 
   {  
      return static_cast< double >( i * 2 );
   } );
   processor.Push( 23 );
   processor.Push( 5 );
   EXPECT_EQ( 46., processor.PopOrWait()->get() );
   EXPECT_EQ( 10., processor.PopOrWait()->get() );
}

TEST( DataProcessor, MultiThreadingOrder )
{
   DataProcessor< int, int > processor( 4, []( int i ) 
   {  
      return i;
   } );
   for ( int no( 0 ); no < 100; ++no )
   {  
      processor.Push( int( no ) ); 
   }
   for ( int no( 0 ); no < 100; ++no )
   {  
      EXPECT_EQ( no, processor.PopOrWait()->get() ); 
   }
}

TEST( DataProcessor, VoidOutput )
{
   std::atomic< int > sum( 0 );
   DataProcessor< int > processor( 1, [ &sum ]( int i ) 
   { 
      sum += i; 
   } );
   processor.Push( 23 );
   processor.Push(  5 );
   EXPECT_NO_THROW( processor.PopOrWait()->get() );
   EXPECT_NO_THROW( processor.PopOrWait()->get() );
   EXPECT_EQ( 28, sum.load() );
}

TEST( DataProcessor, Uncopyable )
{
   DataProcessor< Uncopyable, Uncopyable > processor( 1, []( Uncopyable i ) 
   { 
      i.m_value *= 2; 
      return i; 
   } );
   processor.Push( Uncopyable( 23 ) );
   processor.Push( Uncopyable(  5 ) );
   EXPECT_EQ( 46, processor.PopOrWait()->get().m_value );
   EXPECT_EQ( 10, processor.PopOrWait()->get().m_value );
}

TEST( ContinuationDataProcessor, ConstructDestroy )
{
   DataProcessor< int, int > a( 2, []( int i ) { return i; } );
   ContinuationDataProcessor< int, int > b( 2, a, []( auto i ) { return i.get(); } );
   ContinuationDataProcessor< int, int > c( 2, b, []( auto i ) { return i.get(); } );
}

TEST( ContinuationDataProcessor, PushPopMixedTypes )
{
   DataProcessor< int, float > a( 4, []( int i )
   {
      return static_cast< float >( i * 2 );
   } );
   ContinuationDataProcessor< float, int > b( 4, a, []( std::future< float > i )
   {
      return static_cast< int >( i.get() * 0.5f ); ///< Could throw, but should be forwarded
   } );
   for ( int no( 0 ); no < 100; ++no )
   {
      a.Push( int( no ) );
   }
   for ( int no( 0 ); no < 100; ++no )
   {
      EXPECT_EQ( no, b.PopOrWait()->get() );
   }
   a.Cancel(); ///< Cancelation of the first ensures that all items are processed
   b.Wait();   ///< But we have to wait for the last
}

TEST( ContinuationDataProcessor, VoidContinuation )
{
   std::atomic< int > called( 0 );
   DataProcessor< int, int > a( 2, []( int i )
   {
      return i;
   } );
   ContinuationDataProcessor< int > b( 2, a, [ &called ]( std::future< int > i )
   {
      i.get(); ///< Could throw, but should be forwarded
      ++called;
   } );
   a.Push( 3 );
   a.Push( 5 );
   EXPECT_NO_THROW( b.PopOrWait()->get() );
   EXPECT_NO_THROW( b.PopOrWait()->get() );
   EXPECT_FALSE( b.Pop() );
   EXPECT_EQ( 2, called.load() );
   a.Cancel(); ///< Cancel first
   b.Wait();   ///< Wait for last
}

TEST( ContinuationDataProcessor, Uncopyable )
{
   DataProcessor< int, Uncopyable > a( 2, []( int i )
   {
      return Uncopyable( i );
   } );
   ContinuationDataProcessor< Uncopyable, int > b( 2, a, []( std::future< Uncopyable > i )
   {
      return i.get().m_value; ///< Could throw, but should be forwarded
   } );
   a.Push( 3 );
   a.Push( 5 );
   EXPECT_EQ( 3, b.PopOrWait()->get() );
   EXPECT_EQ( 5, b.PopOrWait()->get() );
   EXPECT_FALSE( b.Pop() );
   a.Cancel(); ///< Cancel first
   b.Wait();   ///< Wait for last
}

TEST( ContinuationDataProcessor, Throw )
{
   DataProcessor< int, int > a( 2, []( int i )
   {
      if ( i == 5 )
      {  throw std::exception(); }
      return i;
   } );
   ContinuationDataProcessor< int, int > b( 2, a, []( std::future< int > i )
   {
      return i.get(); ///< Could throw, but should be forwarded
   } );
   a.Push( 3 );
   a.Push( 5 );
   a.Push( 7 );
   EXPECT_EQ( 3, b.PopOrWait()->get() );
   EXPECT_THROW( b.PopOrWait()->get(), std::exception );
   EXPECT_EQ( 7, b.PopOrWait()->get() );
   EXPECT_FALSE( b.Pop() );
   a.Cancel(); ///< Cancel first
   b.Wait();   ///< Wait for last
}

TEST( ContinuationDataProcessor, ComplexChaining )
{
   DataProcessor< std::list< int >, std::vector< int > > a( 2, []( std::list< int > input )
   {
      return std::vector< int >( input.begin(), input.end() );
   } );
   ContinuationDataProcessor< std::vector< int >, int > b( 2, a, []( std::future< std::vector< int > > _input )
   {
      auto input( _input.get() );
      return std::accumulate( input.begin(), input.end(), 0, []( int i, int v ){ return i + v; } );
   } );
   ContinuationDataProcessor< int, std::list< int > > c( 2, b, []( std::future< int > _input )
   {
      auto input( _input.get() );
      return std::list< int >( { input, input * 2, input * 3 } );
   } );
   a.Push( std::list< int >{ 1, 2, 3 } );
   a.Push( std::list< int >{ 9, 8, 7 } );
   a.Push( std::list< int >{ 23 } );
   EXPECT_EQ( ( std::list< int >{  6, 12, 18 } ), c.PopOrWait()->get() );
   EXPECT_EQ( ( std::list< int >{ 24, 48, 72 } ), c.PopOrWait()->get() );
   EXPECT_EQ( ( std::list< int >{ 23, 46, 69 } ), c.PopOrWait()->get() );
   a.Cancel(); ///< Cancel first
   c.Wait();   ///< Wait for last
}

TEST( ContinuationDataProcessor, Termination )
{
   std::atomic< int > exceptionCount( 0 );
   std::vector< int > values;
   DataProcessor< int, int > a( 2, []( int input )
   {
      return input;
   } );
   ContinuationDataProcessor< int, int > b( 2, a, []( std::future< int > _input )
   {
      auto input( _input.get() );
      if ( input == 5 )
      {  throw std::exception(); }
      return input;
   } );
   TerminationProcessor< int > c( b, [ &values, &exceptionCount ]( std::future< int > input )
   {
      try 
      {  values.emplace_back( input.get() ); }
      catch ( std::exception const& e )
      {  ++exceptionCount; }
   } );
   for ( auto i : { 23, 5, 7, 5, 42 } ) { a.Push( std::move( i ) ); }
   a.Cancel(); ///< Cancel first
   c.Wait();   ///< Wait for last
   EXPECT_EQ( 2, exceptionCount.load() );
   EXPECT_EQ( std::vector< int >( { 23, 7, 42 } ), values );
}

TEST( ContinuationDataProcessor, VoidTermination )
{
   std::atomic< int > exceptionCount( 0 );
   DataProcessor< int, int > a( 2, []( int input )
   {
      return input;
   } );
   ContinuationDataProcessor< int > b( 2, a, []( std::future< int > _input )
   {
      auto input( _input.get() );
      if ( input == 5 )
      {  throw std::exception(); }
   } );
   TerminationProcessor<> c( b, [ &exceptionCount ]( std::future< void > input )
   {
      try { input.get(); }
      catch ( std::exception const& e )
      {  ++exceptionCount; }
   } );
   for ( auto i : { 23, 5, 7, 5, 42 } ) { a.Push( std::move( i ) ); }
   a.Cancel(); ///< Cancel first
   c.Wait();   ///< Wait for last
   EXPECT_EQ( 2, exceptionCount.load() );
}

TEST( TaskAndContinuationDataProcessor, ConstructDestroy )
{
   BufferingTaskProcessor< int > a( 1 );
   ContinuationDataProcessor< int, int > b( 1, a, []( std::future< int > _input ) { return _input.get() * 2; } );
}

TEST( TaskAndContinuationDataProcessor, PushPop )
{
   BufferingTaskProcessor< int > a( 2 );
   ContinuationDataProcessor< int, int > b( 2, a, []( std::future< int > _input ) { return _input.get() * 2; } );
   
   a.Push([] { return 2; });
   a.Push([] { return 23; });
   EXPECT_EQ(  4, b.PopOrWait()->get() );
   EXPECT_EQ( 46, b.PopOrWait()->get() );
   
   a.Cancel(); ///< Cancel first
   b.Wait();   ///< Wait for last
}

TEST( TaskAndContinuationDataProcessor, Termination )
{
   std::atomic<int> exceptionCount( 0 );
   std::vector< int > values;
   BufferingTaskProcessor< int > a( 2 );
   ContinuationDataProcessor< int, int > b( 2, a, []( std::future< int > _input ) 
   { 
      auto v(_input.get());
      if ( v == 5 )
      {  throw std::exception(); }
      return v * 2; 
   } );
   TerminationProcessor< int > c( b, [ &values, &exceptionCount ]( std::future< int > input )
   {
      try 
      {  values.emplace_back( input.get() ); }
      catch ( std::exception const& e )
      {  ++exceptionCount; }
   } );
   a.Push([] { return 2; });
   a.Push([] { return 5; });
   a.Push([] { return 23; });   
   a.Cancel(); ///< Cancel first
   c.Wait();   ///< Wait for last   
   EXPECT_EQ( 1, exceptionCount );
   EXPECT_EQ( std::vector< int >( { 4, 46 } ), values );
}
