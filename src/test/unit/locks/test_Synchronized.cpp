#include "kclpp/locks/Synchronized.h"

#include <gtest/gtest.h>
#include <thread>
#include <atomic>
#include <exception>
#include <stdexcept>
#include <vector>

using kclpp::locks::Synchronized;
using namespace std;

TEST(TestSynchronized, TestOneThread) {
  Synchronized<vector<int>> nums;
  for (int i = 0; i < 10; i++) {
    auto handle = nums.getHandle();
    handle->push_back(i);
  }
  int result = nums.getHandle()->back();
  EXPECT_EQ(9, result);
}


TEST(TestSynchronized, TestMoreThreads) {
  Synchronized<vector<int>> nums;
  {
    vector<thread*> threads;
    for (int i = 0; i < 20; i++) {
      threads.push_back(new thread([&nums, i]() {
        for (int j = 0; j < 1000; j++) {
          int val = i * j;
          auto handle = nums.getHandle();
          handle->push_back(val);
        }
      }));
    }
    for (auto t : threads) {
      t->join();
    }
    for (auto t: threads) {
      delete t;
    }
  }
  int result = nums.getHandle()->size();
  EXPECT_EQ(20000, result);
}
