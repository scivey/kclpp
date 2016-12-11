#include "kclpp/async/tpool/ThreadPool.h"
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

ThreadPool::ThreadPool(size_t numThreads, std::unique_ptr<queue_t> workQueue)
    : numThreads_(numThreads), workQueue_(std::move(workQueue)){}

ThreadPool* ThreadPool::createNew(size_t nThreads) noexcept {
  return new ThreadPool{
    nThreads,
    util::makeUnique<queue_t>(size_t{50000})
  };
}

bool ThreadPool::isRunning() const noexcept {
  return running_.load();
}

using stop_outcome_t = ThreadPool::stop_outcome_t;

stop_outcome_t ThreadPool::triggerStop() noexcept {
  for (;;) {
    if (!running_.load()) {
      return stop_outcome_t {
        NotRunning {"NotRunning"}
      };
    }
    bool expected = true;
    bool desired = false;
    if (running_.compare_exchange_strong(expected, desired)) {
      stopWorkers();
      return stop_outcome_t { Unit{} };
    }
  }
}

void ThreadPool::stopWorkers() noexcept {
  for (auto& worker: workers_) {
    worker->triggerStop();
  }
}

void ThreadPool::startWorkers() noexcept {
  DCHECK(numThreads_ >= 1);
  for (size_t i = 0; i < numThreads_; i++) {
    auto worker = std::make_shared<ThreadPoolWorker>(
      workQueue_.get()
    );
    auto startOutcome = worker->start();
    DCHECK(startOutcome.IsSuccess()) << startOutcome.GetError().what();
    workers_.push_back(std::move(worker));
  }
}

using start_outcome_t = ThreadPool::start_outcome_t;

start_outcome_t ThreadPool::start() noexcept {
  if (numThreads_ < 1) {
    return start_outcome_t {
      InvalidSettings {"ThreadPool needs at least 1 thread."}
    };
  }
  for (;;) {
    if (running_.load()) {
      return start_outcome_t {
        AlreadyRunning {"Already running."}
      };
    }
    bool expected = false;
    bool desired = true;
    if (running_.compare_exchange_strong(expected, desired)) {
      startWorkers();
      return start_outcome_t{Unit{}};
    }
  }
}

void ThreadPool::join() noexcept {
  for (auto& worker: workers_) {
    worker->join();
  }
}

stop_outcome_t ThreadPool::stopJoin() noexcept {
  auto stopResult = triggerStop();
  if (!stopResult.IsSuccess()) {
    return stopResult;
  }
  join();
  return stopResult;
}

using submit_result_t = ThreadPool::submit_result_t;

submit_result_t ThreadPool::trySubmit(std::unique_ptr<Task>&& task) noexcept {
  if (!isRunning()) {
    return submit_result_t{NotRunning{"Not running."}};
  }
  if (workQueue_->try_enqueue(std::move(task))) {
    return submit_result_t{Unit{}};
  }
  return submit_result_t{ThreadPoolError{"Queue is full."}};
}

ThreadPool::~ThreadPool() {
  stopJoin(); // ignore potential NotRunning error
}

}}} // kclpp::async::tpool
