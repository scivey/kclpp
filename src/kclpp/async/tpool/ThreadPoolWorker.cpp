#include "kclpp/async/tpool/ThreadPoolWorker.h"
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

using sender_channel_t = ThreadPoolWorker::sender_channel_t;

sender_channel_t& ThreadPoolWorker::getDataChannel(sender_id_t senderId,
    EventContext *ctx) noexcept {
  auto found = dataChannels_.find(senderId);
  if (found != dataChannels_.end()) {
    return found->second;
  }
  auto newChannel = EventDataChannel::createSharedAsSender();
  DCHECK(ctx->threadsafeRegisterDataChannel(newChannel).IsSuccess());
  while (!newChannel->hasReceiverAcked()) {
    ;
  }
  dataChannels_.insert(std::make_pair(senderId, newChannel));
  found = dataChannels_.find(senderId);
  DCHECK(found != dataChannels_.end());
  return found->second;
}

using run_outcome_t = ThreadPoolWorker::run_outcome_t;

run_outcome_t ThreadPoolWorker::runTask(std::unique_ptr<Task> task) noexcept {
  run_outcome_t taskOutcome;
  try {
    task->run();
    taskOutcome = run_outcome_t{Unit{}};
  } catch (const std::exception& ex) {
    taskOutcome = run_outcome_t {
      TaskThrewException {ex.what()}
    };
  }
  auto senderContext = task->getSenderContext();
  auto senderId = task->getSenderId();
  auto wrappedTask = makeMoveWrapper(std::move(task));
  auto wrappedOutcome = makeMoveWrapper(std::move(taskOutcome));
  EventDataChannel::Message message {
    [wrappedTask, wrappedOutcome]() {
      MoveWrapper<run_outcome_t> movedOutcome = wrappedOutcome;
      run_outcome_t unwrappedOutcome = movedOutcome.move();
      MoveWrapper<std::unique_ptr<Task>> movedTask = wrappedTask;
      std::unique_ptr<Task> unwrappedTask = movedTask.move();
      if (unwrappedOutcome.IsSuccess()) {
        unwrappedTask->onSuccess();
      } else {
        unwrappedTask->onError(unwrappedOutcome.GetError());
      }
    }
  };
  auto dataChannel = getDataChannel(senderId, senderContext);
  auto sendResult = dataChannel->getQueue()->trySend(std::move(message));
  if (!sendResult.IsSuccess()) {
    return run_outcome_t {
      CouldntSendResult {
        sendResult.GetError().what()
      }
    };
  }
  return run_outcome_t{Unit{}};
}

void ThreadPoolWorker::threadFunc() noexcept {
  std::unique_ptr<Task> task;
  const int64_t kWaitMicroseconds {1000}; // 1ms
  const size_t kFullLoadFreq {20};
  bool isRunning = running_.load();
  size_t fullLoadCounter {0};
  while (isRunning) {
    fullLoadCounter++;
    if (workQueue_->wait_dequeue_timed(task, kWaitMicroseconds)) {
      if (task->valid()) {
        DCHECK(runTask(std::move(task)).IsSuccess());
      } else {
        LOG(INFO) << "received invalid task.";
      }
    }
    // we do a relaxed atomic load most of the time,
    // since it's cheaper.
    // ever `kFullLoadFreq` loops, we do a full
    // memory_order_seq_cst load.
    if (fullLoadCounter > 0 && fullLoadCounter % kFullLoadFreq == 0) {
      isRunning = running_.load();
      fullLoadCounter = 0;
    } else {
      isRunning = running_.load(std::memory_order_relaxed);
    }
  }
}

void ThreadPoolWorker::startThread() noexcept {
  DCHECK(running_.load());
  DCHECK(!thread_);
  thread_.reset(new std::thread([this]() {
    this->threadFunc();
  }));
}

ThreadPoolWorker::ThreadPoolWorker(queue_t *workQueue): workQueue_(workQueue){}

ThreadPoolWorker* ThreadPoolWorker::createNew(queue_t *workQueue) noexcept {
  return new ThreadPoolWorker(workQueue);
}

using start_outcome_t = ThreadPoolWorker::start_outcome_t;

start_outcome_t ThreadPoolWorker::start() noexcept {
  for (;;) {
    if (running_.load()) {
      return start_outcome_t {
        AlreadyRunning {"Already running."}
      };
    }
    bool expected = false;
    bool desired = true;
    if (running_.compare_exchange_strong(expected, desired)) {
      startThread();
      return start_outcome_t {Unit{}};
    }
  }
}

bool ThreadPoolWorker::isRunning() const noexcept {
  return running_.load();
}

using stop_outcome_t = ThreadPoolWorker::stop_outcome_t;

stop_outcome_t ThreadPoolWorker::triggerStop() noexcept {
  for (;;) {
    if (!running_.load()) {
      return stop_outcome_t {
        NotRunning {"NotRunning"}
      };
    }
    bool expected = true;
    bool desired = false;
    if (running_.compare_exchange_strong(expected, desired)) {
      return stop_outcome_t {Unit{}};
    }
  }
}

void ThreadPoolWorker::join() noexcept {
  if (thread_) {
    thread_->join();
  }
}


stop_outcome_t ThreadPoolWorker::stopJoin() noexcept {
  auto stopResult = triggerStop();
  if (!stopResult.IsSuccess()) {
    return stopResult;
  }
  join();
  return stopResult;
}

}}} // kclpp::async::tpool
