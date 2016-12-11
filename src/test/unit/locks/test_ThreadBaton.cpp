#include <gtest/gtest.h>
#include <thread>
#include "kclpp/locks/ThreadBaton.h"

using namespace std;
using kclpp::locks::ThreadBaton;

TEST(TestThreadBaton, TestSanity1) {
  ThreadBaton baton;
  EXPECT_FALSE(baton.isDone());
  baton.post();
  EXPECT_TRUE(baton.isDone());
}

TEST(TestThreadBaton, TestSanity2) {
  ThreadBaton baton1, baton2, baton3;
  EXPECT_FALSE(baton1.isDone());
  EXPECT_FALSE(baton2.isDone());
  EXPECT_FALSE(baton3.isDone());

  std::atomic<size_t> currentState {10};

  thread t1([&baton1, &baton2, &baton3, &currentState]() {
    EXPECT_EQ(10, currentState.load());
    currentState.store(100);
    baton1.post();
    baton2.wait();
    EXPECT_EQ(1000, currentState.load());
    currentState.store(10000);
    baton3.post();
  });
  std::atomic<bool> finished {false};
  thread t2([&baton1, &baton2, &baton3, &currentState, &finished]() {
    baton1.wait();
    EXPECT_EQ(100, currentState.load());
    currentState.store(1000);
    baton2.post();
    baton3.wait();
    EXPECT_EQ(10000, currentState.load());
    finished.store(true);
  });
  t2.join();
  EXPECT_TRUE(finished);
  t1.join();
}