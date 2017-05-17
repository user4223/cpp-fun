
#include "../include/Semaphore.h"

#include <gtest/gtest.h>

#include <future>

TEST( Semaphore, CreateDestroy )
{
   Semaphore s(3);
}

TEST( Semaphore, InvalidCount )
{
   EXPECT_THROW(Semaphore(0), std::invalid_argument);
}

TEST( Semaphore, NotBlockingUntilMaximum )
{
   Semaphore s(3);
   auto token1(s.acquire());
   auto token2(s.acquire());
   auto token3(s.acquire());
   EXPECT_TRUE(token1);
   EXPECT_TRUE(token2);
   EXPECT_TRUE(token3);
}

TEST( Semaphore, BlockingOnMaximum )
{
   Semaphore s(2);
   std::future<void> third;
   std::atomic<bool> called(false);
   auto token1(s.acquire());
   EXPECT_TRUE(token1);
   {
      auto token2(s.acquire());
      EXPECT_TRUE(token2);
      third = std::async(std::launch::async, [&]
      { 
         auto token3(s.acquire());
         EXPECT_TRUE(token3);
         called = true; 
      });
      std::this_thread::yield();
      EXPECT_FALSE(called);
   }
   third.get();
   EXPECT_TRUE(called);
}

TEST( Semaphore, Timeout )
{
   Semaphore s(2);
   auto token1(s.acquire());
   EXPECT_TRUE(token1);
   {
      auto token2(s.acquire());
      EXPECT_TRUE(token2);
      EXPECT_FALSE(s.acquire(std::chrono::microseconds(10)));
   }
   EXPECT_TRUE(s.acquire(std::chrono::microseconds(10)));
}
