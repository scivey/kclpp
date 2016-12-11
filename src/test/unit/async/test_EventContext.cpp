#include <gtest/gtest.h>
#include <thread>
#include <atomic>
#include <chrono>
#include "kclpp/async/EventContext.h"
#include "kclpp/async/queues/EventDataChannel.h"

using namespace kclpp;
using namespace kclpp::async;
using kclpp::async::queues::EventDataChannel;
namespace util = kclpp::util;
using namespace std;
using ControlMessage = EventContext::ControlMessage;

struct TestMessage {
  thread::id sentFrom {0};
  thread::id ranIn {0};
  TestMessage(){}
  TestMessage(thread::id sender, thread::id runner)
    : sentFrom(sender), ranIn(runner) {}
};

TEST(TestEventContext, TestControlMessages) {
  auto evCtx = kclpp::util::createShared<EventContext>();
  std::atomic<bool> done {false};
  std::vector<TestMessage> results;
  std::thread::id baseId = this_thread::get_id();

  const size_t kNumThreads = 8;
  std::vector<std::shared_ptr<thread>> threads;
  for (size_t i = 0; i < kNumThreads; i++) {
    threads.push_back(std::make_shared<std::thread>(
      [evCtx, &results, kNumThreads, baseId, &done]() {
        auto senderId = this_thread::get_id();
        auto sendOutcome = evCtx->threadsafeTrySendControlMessage(ControlMessage{
          [senderId, &results, baseId, kNumThreads, &done]() {
            auto runnerId = this_thread::get_id();
            EXPECT_EQ(baseId, runnerId);
            results.push_back(TestMessage{senderId, runnerId});
            if (results.size() >= kNumThreads) {
              done.store(true);
            }
          }
        });
        EXPECT_TRUE(sendOutcome.IsSuccess());
      }
    ));
  }
  while (!done.load()) {
    evCtx->getBase()->runOnce();
  }
  for (auto& t: threads) {
    t->join();
  }
  EXPECT_EQ(kNumThreads, results.size());
  std::set<thread::id> seen;
  for (auto& msg: results) {
    EXPECT_EQ(baseId, msg.ranIn);
    seen.insert(msg.sentFrom);
  }
  EXPECT_EQ(kNumThreads, seen.size());
  EXPECT_EQ(0, seen.count(baseId));
}

class DataChannelProducer {
 public:
  using channel_t = EventDataChannel;
  using channel_ptr_t = std::shared_ptr<channel_t>;
 protected:
  channel_ptr_t channel_ {nullptr};
 public:
  DataChannelProducer(channel_ptr_t chan): channel_(chan){}
  static DataChannelProducer* createNew() {
    return new DataChannelProducer{
      channel_t::createSharedAsSender()
    };
  }
  channel_ptr_t getChannel() {
    return channel_;
  }
  void waitForConsumer() {
    while (!channel_->hasReceiverAcked()) {
      ;
    }
  }
};

TEST(TestEventContext, TestRegisterDataChannel) {
  auto evCtx = kclpp::util::createShared<EventContext>();
  std::atomic<bool> done {false};
  std::thread::id evLoopTid = this_thread::get_id();
  std::shared_ptr<DataChannelProducer> producerPtr {nullptr};
  thread prodThread([&producerPtr, evCtx, &done, evLoopTid]() {
    auto prod = kclpp::util::createShared<DataChannelProducer>();
    producerPtr = prod;
    evCtx->threadsafeRegisterDataChannel(prod->getChannel());
    prod->waitForConsumer();
    std::atomic<bool> messageHandled {false};
    auto producerTid = this_thread::get_id();
    auto outcome = prod->getChannel()->getQueue()->trySend(EventDataChannel::Message{
      [&messageHandled, evLoopTid, producerTid]() {
        EXPECT_EQ(evLoopTid, this_thread::get_id());
        EXPECT_NE(producerTid, this_thread::get_id());
        messageHandled.store(true);
      }
    });
    EXPECT_TRUE(outcome.IsSuccess());
    while (!messageHandled.load()) {
      ;
    }
    done.store(true);
  });
  while (!done.load()) {
    evCtx->getBase()->runOnce();
  }
  prodThread.join();
  EXPECT_TRUE(done.load());
}
