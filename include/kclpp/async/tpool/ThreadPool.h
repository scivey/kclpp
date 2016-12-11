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
#include "kclpp/async/tpool/ThreadPoolWorker.h"

namespace kclpp { namespace async { namespace tpool {


class ThreadPool {
 public:
  using queue_t = queues::MPMCQueue<std::unique_ptr<Task>>;
 protected:
  size_t numThreads_ {0};
  std::unique_ptr<queue_t> workQueue_ {nullptr};
  std::vector<std::shared_ptr<ThreadPoolWorker>> workers_;
  std::atomic<bool> running_ {false};
  ThreadPool(size_t numThreads, std::unique_ptr<queue_t> workQueue);

 public:
  static ThreadPool* createNew(size_t nThreads) noexcept;
  bool isRunning() const noexcept;
  using stop_outcome_t = Outcome<Unit, ThreadPoolError>;
  stop_outcome_t triggerStop() noexcept;
 protected:
  void stopWorkers() noexcept;
  void startWorkers() noexcept;
 public:
  using start_outcome_t = Outcome<Unit, ThreadPoolError>;
  start_outcome_t start() noexcept;
  void join() noexcept;
  stop_outcome_t stopJoin() noexcept;
  using submit_result_t = Outcome<Unit, ThreadPoolError>;
  submit_result_t trySubmit(std::unique_ptr<Task>&& task) noexcept;
  ~ThreadPool();
};

}}} // kclpp::async::tpool
