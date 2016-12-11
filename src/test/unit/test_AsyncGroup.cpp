#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <memory>
#include <atomic>
#include "kclpp/AsyncGroup.h"


using namespace std;
using kclpp::AsyncGroup;


TEST(TestAsyncGroup, TestSanity1) {
  AsyncGroup group {2};
  EXPECT_FALSE(group.isDone());
  group.post();
  EXPECT_FALSE(group.isDone());
  group.post();
  EXPECT_TRUE(group.isDone());
  EXPECT_TRUE(group.becomeLeader());
  EXPECT_FALSE(group.becomeLeader());
  EXPECT_TRUE(group.isDone());
}

TEST(TestAsyncGroup, TestSanity2) {
  for (size_t i = 0; i < 100; i++) {
    std::atomic<size_t> counter {0};
    const size_t kGroupSize = 4;
    AsyncGroup group {kGroupSize};
    EXPECT_FALSE(group.isDone());
    vector<unique_ptr<thread>> threads;
    for (size_t i = 0; i < kGroupSize; i++) {
      threads.emplace_back(new thread {[&group, &counter]() {
        group.post();
        if (group.isDone() && group.becomeLeader()) {
          counter.fetch_add(1);
        }
      }});
    }
    while (!group.isDone()) {
      ;
    }
    for (auto& thread: threads) {
      thread->join();
      thread.reset();
    }
    threads.clear();
    EXPECT_EQ(1, counter.load());
  }
}