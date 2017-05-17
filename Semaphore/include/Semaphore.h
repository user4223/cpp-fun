#pragma once

#include <boost/optional/optional.hpp>

#include <mutex>
#include <functional>
#include <stdexcept>
#include <chrono>
#include <condition_variable>
 
/** Simple counting semaphore
 */
struct Semaphore
{
   struct Token
   {
      Token(std::function<void()> doRelease) : m_doRelease(doRelease) {}
      
      ~Token() { release(); }
      
      Token() = default;
      Token(Token&) = delete;
      Token& operator=(Token&) = delete;
      Token(Token&&) = default;
      Token& operator=(Token&&) = default;
      
      void release() { if (m_doRelease) { m_doRelease(); } }
      
   private:
      std::function<void()> m_doRelease;
   };
   
   Semaphore(size_t count) : 
       m_maximumCount(count)
      ,m_currentCount(count)
      ,m_mutex()
      ,m_condition()
   {
      if (m_maximumCount == 0)
      {  throw std::invalid_argument("Semaphore count cannot equal 0"); }
   }
   
   boost::optional<Token> acquire()
   {  return acquire(std::chrono::microseconds::max()); }
   
   boost::optional<Token> acquire(std::chrono::microseconds timeout)
   {
      std::unique_lock<std::mutex> lock(m_mutex);
      if (!m_condition.wait_for(lock, timeout, [this]{ return m_currentCount > 0; }))
      {  return boost::optional<Token>(); }
      --m_currentCount;
      return boost::optional<Token>(Token([this]{ release(); }));
   }
   
private:
   void release()
   {
      std::unique_lock<std::mutex> lock(m_mutex);
      ++m_currentCount;
      m_condition.notify_all();
   }
   
private:
   size_t m_maximumCount;
   size_t m_currentCount;
   mutable std::mutex m_mutex;
   std::condition_variable m_condition;
};
 