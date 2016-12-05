#pragma once

#include <vector>
#include <future>
#include <algorithm>
#include <atomic>
#include <queue>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <condition_variable>
#include <experimental/optional>

namespace
{
   /** Workaround for overflow bug: http://stackoverflow.com/questions/27726818/stdcondition-variablewait-for-exits-immediately-when-given-stdchronodura
    * */
   template < typename DurationType >
   auto GetMax()
   {
      return std::chrono::duration_cast< DurationType >( std::chrono::hours( 8736 ) /* one year */ );
   }
   
   template < typename FunctionT, typename... ArgumentT >
   auto CreateWorker( size_t workerCount, FunctionT&& function, ArgumentT&&... arguments )
   {
      std::vector< std::future< void > > worker;
      for ( size_t i( 0 ); i < workerCount; ++i )
      {  worker.emplace_back( std::async( std::launch::async, function, std::forward<ArgumentT>(arguments)... ) ); }
      return std::move( worker );
   }
   
   template < typename WorkerT >
   void JoinWorker( WorkerT&& worker )
   {
      std::for_each( worker.begin(), worker.end(), []( std::future< void >& future )
      {  future.get(); } );
   }
}
 
template < typename T >
struct Queue
{
   typedef T value_type;
   typedef std::experimental::optional< value_type > optional_value_type;
   
   Queue() : 
      m_canceled( false )
     ,m_queue()
     ,m_mutex()
     ,m_condition() 
   {}
   
   ~Queue()
   {
      Cancel();
   }
   
   bool IsCanceled() const
   {
      std::unique_lock< std::mutex > lock( m_mutex );
      return m_canceled;
   }
   
   void Cancel()
   {
      std::unique_lock< std::mutex > lock( m_mutex );
      m_canceled = true;
      m_condition.notify_all();
   }
   
   void Push( T&& item )
   {
      std::unique_lock< std::mutex > lock( m_mutex );
      if ( m_canceled )
      {  throw std::logic_error( "Queue already canceled" ); }
      
      m_queue.emplace( std::move( item ) );
      m_condition.notify_one();
   }
   
   optional_value_type Pop()
   {
      std::unique_lock< std::mutex > lock( m_mutex );
      if ( m_queue.empty() )
      {  return optional_value_type(); }
      
      auto r( std::move( m_queue.front() ) );
      m_queue.pop();
      return std::move( r );
   }
            
   template < typename DurationType = std::chrono::seconds >
   optional_value_type PopOrWait( DurationType duration = GetMax< DurationType >() )
   {
      std::unique_lock< std::mutex > lock( m_mutex );
      if ( m_queue.empty() )
      {
         m_condition.wait_for( lock, duration, [this]
         { return m_canceled || !m_queue.empty(); } );
         
         if ( m_queue.empty() )
         {  return optional_value_type(); }
      }
      
      auto r( std::move( m_queue.front() ) );
      m_queue.pop();
      return std::move( r );
   }
   
private:
   bool m_canceled;
   std::queue< value_type > m_queue;
   mutable std::mutex m_mutex;
   std::condition_variable m_condition;
};
   
/** This is considered as an internal helper class
 *  and not for client use.
 *  All methods should NOT use the mutex member internally,
 *  it has to be used by clients for in a wider scope.
 * */
template < typename T >
struct ProcessorBase
{
   typedef T value_type;
   typedef Queue< T > queue_type;
   
   ProcessorBase() : m_output(), m_mutex() {}
                             
   typename queue_type::optional_value_type Pop()
   {  return std::move( m_output.Pop() ); }
      
   template < typename DurationType = std::chrono::seconds >
   typename queue_type::optional_value_type PopOrWait( DurationType duration = GetMax< DurationType >() )
   {  return std::move( m_output.PopOrWait( duration ) ); }
   
   auto Lock() const
   {  return std::move( std::unique_lock< std::mutex >( m_mutex ) ); }
   
   queue_type m_output;
   mutable std::mutex m_mutex;
};

template < typename QueueT >
struct TaskWorker
{
   TaskWorker( QueueT& queue ) : m_queue( queue ) {}
   
   void operator()()
   {
      while ( !m_queue.IsCanceled() )
      {
         /** The timeout here avoids a deadlock when between 
          *  IsCanceled and Pop the internal queue state changes
          * */
         auto item( m_queue.PopOrWait( std::chrono::seconds( 1 ) ) );
         if ( item ) { item.value()(); }
      }
      while ( 1 ) ///< Canceled but we finish all enqueued work before we leave
      {
         auto item( m_queue.Pop() ); ///< Without wait, there cannot be new items, we only take what is already there
         if ( !item ) { break; }
         item.value()(); 
      }
   }
   
private:
   QueueT& m_queue;
};

template < typename T = void >
struct TaskProcessor : ProcessorBase< std::packaged_task< T() > >
{
   typedef T value_type;
   typedef ProcessorBase< std::packaged_task< value_type() > > base_type;
   typedef typename base_type::queue_type output_queue_type;
        
   TaskProcessor( size_t workerCount ) :
       base_type()
      ,m_worker( CreateWorker( 
          workerCount
         ,TaskWorker< output_queue_type >( this->m_output ) ) )
   {}
   
   ~TaskProcessor()
   {
      Cancel();
      Wait();
   }
   
   template < typename FunctionT >
   std::future< value_type > Push( FunctionT&& function )
   {
      auto lock( this->Lock() );       
      std::packaged_task< value_type() > task( std::move( function ) );
      auto future( task.get_future() );
      this->m_output.Push( std::move( task ) );
      return std::move( future );
   }
              
   void Cancel()
   {
      auto lock( this->Lock() );
      this->m_output.Cancel();
   }
   
   void Wait()
   {
      /** This is not thread save */
      if ( !m_worker.empty() )
      {  JoinWorker( std::move( m_worker ) ); }
   }
         
private:
   std::vector< std::future< void > > m_worker;
};
   
template < typename T = void >
struct BufferingTaskProcessor : ProcessorBase< std::future< T > >
{
   typedef T value_type;
   typedef ProcessorBase< std::future< T > > base_type;
   typedef Queue< std::packaged_task< value_type() > > input_queue_type;
   typedef typename base_type::queue_type output_queue_type;
   
   using base_type::Pop;
   using base_type::PopOrWait;
   
   BufferingTaskProcessor( size_t workerCount ) :
       base_type()
      ,m_input()
      ,m_worker( CreateWorker( 
          workerCount
         ,TaskWorker< input_queue_type >( this->m_input ) ) )
   {}
   
   ~BufferingTaskProcessor()
   {
      Cancel();
      Wait();
   }
   
   template < typename FunctionT >
   void Push( FunctionT&& function )
   {
      auto lock( this->Lock() );       
      std::packaged_task< value_type() > task( std::move( function ) );
      this->m_output.Push( std::move( task.get_future() ) );
      this->m_input.Push( std::move( task ) );
   }
              
   void Cancel()
   {
      auto lock( this->Lock() );
      this->m_output.Cancel();
      this->m_input.Cancel();
   }
   
   void Wait()
   {
      /** This is not thread save */
      if ( !m_worker.empty() )
      {  JoinWorker( std::move( m_worker ) ); }
   }
         
private:
   input_queue_type m_input;
   std::vector< std::future< void > > m_worker;
};
  
template < typename InputT, typename OutputT = void >
struct DataProcessor : BufferingTaskProcessor< OutputT >
{
   typedef BufferingTaskProcessor< OutputT > base_type;
   typedef std::function< OutputT( InputT&& ) > function_type;
   
   using base_type::Cancel;
   using base_type::Pop;
   using base_type::PopOrWait;
   
   DataProcessor( size_t workerCount, function_type function ) :
       base_type( workerCount )
      ,m_function( function )
   {}
   
   void Push( InputT&& data )
   {
      base_type::Push( std::bind( m_function, std::bind( std::move< InputT& >, std::move( data ) ) ) );
   }

private:
   function_type m_function;
};

template < typename QueueT, typename ContinuationT >
struct ContinuationDataWorker
{
   ContinuationDataWorker( std::atomic< bool >& canceled, QueueT& queue, ContinuationT& continuation ) : m_canceled( canceled ), m_queue( queue ), m_continuation( continuation ) {}
   
   void operator()()
   {
      while ( !m_canceled.load() ) ///< When we got canceled directly, we just stop working here
      {               
         if ( m_queue.IsCanceled() ) ///< Predecessor is canceled, we takover all results and cancel then as well
         {
            while ( 1 )
            {
               auto item( m_queue.Pop() ); ///< Without wait, because new items cannot be added anymore
               if ( !item ) { break; }
               m_continuation.Push( std::move( item.value() ) );
            }
            break;
         }
          
         /** The timeout here avoids a deadlock when between 
          *  IsCanceled and Pop the internal queue state changes
          * */
         auto item( m_queue.PopOrWait( std::chrono::seconds( 1 ) ) );
         if ( item ) 
         {  m_continuation.Push( std::move( item.value() ) ); }
      }
      m_continuation.Cancel(); ///< We cancel the queues not until here when the thread finishes
   }
   
private:
   std::atomic< bool >& m_canceled;
   QueueT& m_queue;
   ContinuationT& m_continuation;
};

template < typename InputT, typename OutputT = void >
struct ContinuationDataProcessor : DataProcessor< std::future< InputT >, OutputT >
{
   typedef DataProcessor< std::future< InputT >, OutputT > base_type;
   typedef BufferingTaskProcessor< InputT > predecessor_type;
   
   using typename base_type::function_type;
   using base_type::Pop;
   using base_type::PopOrWait;
   
   ContinuationDataProcessor( size_t workerCount, predecessor_type& predecessor, function_type function ) :
       base_type( workerCount, function )
      ,m_canceled( false )
      ,m_worker( std::async( 
          std::launch::async
         ,ContinuationDataWorker< typename predecessor_type::output_queue_type, base_type >( 
             m_canceled
            ,predecessor.m_output
            ,*this ) ) )
   {}
   
   ~ContinuationDataProcessor()
   {
      Cancel();
      Wait();
   }
   
   void Cancel()
   {
      /** We do not cancel the queues here directly, 
       *  because processing of already enqueued 
       *  items from predecessor would fail then.
       *  Cancelation of the queue is done in scheduler 
       *  thread right before termination.
       */
      m_canceled.store( true );
   }
   
   void Wait()
   {
      /** This is not thread save */
      if ( m_worker.valid() )
      {  m_worker.get(); }
   }

private:
   std::atomic< bool > m_canceled;
   std::future< void > m_worker;
};

template < typename QueueT, typename FunctionT >
struct TerminationWorker
{
   TerminationWorker( std::atomic< bool >& canceled, QueueT& queue, FunctionT&& function ) : m_canceled( canceled ), m_queue( queue ), m_function( function ) {}
   
   void operator()()
   {
      while ( !m_canceled.load() ) ///< When we got canceled directly, we just stop working here
      {               
         if ( m_queue.IsCanceled() ) ///< Predecessor is canceled, we takeover all results and cancel then as well
         {
            while ( 1 )
            {
               auto item( m_queue.Pop() ); ///< Without wait, because new items cannot be added anymore
               if ( !item ) { break; }     ///< The first empty item breaks
               m_function( std::move( item.value() ) );
            }
            break;
         }
          
         /** The timeout here avoids a deadlock when between 
          *  IsCanceled and Pop the internal queue state changes
          * */
         auto item( m_queue.PopOrWait( std::chrono::seconds( 1 ) ) );
         if ( item ) 
         {  m_function( std::move( item.value() ) ); }
      }
   }
   
private:
   std::atomic< bool >& m_canceled;
   QueueT& m_queue;
   FunctionT m_function;
};

template < typename InputT  = void >
struct TerminationProcessor
{
   typedef BufferingTaskProcessor< InputT > predecessor_type;
   typedef std::function< void( std::future< InputT > ) > function_type;
   
   TerminationProcessor( predecessor_type& predecessor, function_type&& function ) :
       m_canceled( false )
      ,m_worker( std::async( 
          std::launch::async
         ,TerminationWorker< typename predecessor_type::output_queue_type, function_type >( 
             m_canceled
            ,predecessor.m_output
            ,std::forward< function_type >( function ) ) ) )
   {}
   
   ~TerminationProcessor()
   {
      Cancel();
      Wait();
   }
   
   void Cancel()
   {
      m_canceled.store( true );
   }
   
   void Wait()
   {
      /** This is not thread save */
      if ( m_worker.valid() )
      {  m_worker.get(); }
   }

private:
   std::atomic< bool > m_canceled;
   std::future< void > m_worker;
};
