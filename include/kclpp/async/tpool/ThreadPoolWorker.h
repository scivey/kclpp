#pragma once
#include <thread>
#include <chrono>
#include "kclpp/func/Function.h"
#include "kclpp/util/misc.h"
#include "kclpp/MoveWrapper.h"
#include "kclpp/KCLPPError.h"
#include "kclpp/async/EventError.h"
#include "kclpp/async/queues/QueueError.h"
#include "kclpp/async/EventContext.h"
#include "kclpp/async/queues/MPMCQueue.h"
#include "kclpp/async/queues/EventDataChannel.h"
#include "kclpp/async/tpool/ThreadPoolError.h"
#include "kclpp/async/tpool/Task.h"

namespace kclpp { namespace async { namespace tpool {


class ThreadPoolWorker {
 public:
  using queue_t = queues::MPMCQueue<std::unique_ptr<Task>>;
  using EventDataChannel = queues::EventDataChannel;
  using sender_channel_t = std::shared_ptr<EventDataChannel>;
  using run_outcome_t = Outcome<Unit, ThreadPoolError>;

 protected:
  using sender_id_t = std::thread::id;
  using data_channel_map_t = std::unordered_map<sender_id_t, sender_channel_t>;
  queue_t *workQueue_ {nullptr};
  std::unique_ptr<std::thread> thread_ {nullptr};
  std::atomic<bool> running_ {false};
  data_channel_map_t dataChannels_;

  sender_channel_t& getDataChannel(sender_id_t senderId, EventContext *ctx) noexcept;
  run_outcome_t runTask(std::unique_ptr<Task> task) noexcept;
  void threadFunc() noexcept;
  void startThread() noexcept;
 public:
  ThreadPoolWorker(queue_t *workQueue);
  static ThreadPoolWorker* createNew(queue_t *workQueue) noexcept;
  using start_outcome_t = Outcome<Unit, ThreadPoolError>;
  start_outcome_t start() noexcept;
  bool isRunning() const noexcept;
  using stop_outcome_t = Outcome<Unit, ThreadPoolError>;
  stop_outcome_t triggerStop() noexcept;
  void join() noexcept;
  stop_outcome_t stopJoin() noexcept;
};

}}} // kclpp::async::tpool
