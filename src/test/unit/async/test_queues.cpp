#include <thread>
#include <chrono>
#include "kclpp/async/queues/SPSCQueue.h"
#include "kclpp/async/queues/MPMCQueue.h"
#include <gtest/gtest.h>

namespace queues = kclpp::async::queues;
using namespace std;

TEST(Test_queues, TestSPSC1) {
  queues::SPSCQueue<int> numQueue {10000};
  const int kNumItems {10};
  thread producer([kNumItems, &numQueue]() {
    for (int i = 0; i < kNumItems; i++) {
      EXPECT_TRUE(numQueue.try_enqueue(i * 10));
    }
  });
  vector<int> received;
  thread consumer([kNumItems, &numQueue, &received]() {
    size_t seen = 0;
    int item;
    while (seen < kNumItems) {
      if (numQueue.try_dequeue(item)) {
        seen++;
        received.push_back(item);
      }
    }
  });
  producer.join();
  consumer.join();
  vector<int> expected {
    0, 10, 20, 30, 40, 50, 60, 70, 80, 90
  };
  EXPECT_EQ(expected, received);
}

TEST(Test_queues, TestMPMC1) {
  queues::MPMCQueue<int> numQueue {10000};
  const int kNumItems {10};
  thread producer([kNumItems, &numQueue]() {
    for (int i = 0; i < kNumItems; i++) {
      EXPECT_TRUE(numQueue.try_enqueue(i * 10));
    }
  });
  vector<int> received;
  thread consumer([kNumItems, &numQueue, &received]() {
    size_t seen = 0;
    int item;
    while (seen < kNumItems) {
      if (numQueue.try_dequeue(item)) {
        seen++;
        received.push_back(item);
      }
    }
  });
  producer.join();
  consumer.join();
  vector<int> expected {
    0, 10, 20, 30, 40, 50, 60, 70, 80, 90
  };
  EXPECT_EQ(expected, received);
}
