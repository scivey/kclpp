#pragma once
#include <thread>
#include <chrono>
#include "kclpp/func/Function.h"
#include "kclpp/util/misc.h"
#include "kclpp/MoveWrapper.h"
#include "kclpp/Unit.h"
#include "kclpp/Outcome.h"
#include "kclpp/async/queues/QueueError.h"
#include "kclpp/async/EventContext.h"
#include "kclpp/async/tpool/ThreadPoolError.h"
#include "kclpp/async/tpool/Task.h"

namespace kclpp { namespace async { namespace tpool {

class CallbackTask: public Task {
 public:
  using work_cb_t = func::Function<void>;
  using done_outcome_t = Outcome<Unit, ThreadPoolError>;
  using done_cb_t = func::Function<void, done_outcome_t>;
 protected:
  work_cb_t work_;
  done_cb_t onFinished_;
 public:
  CallbackTask(work_cb_t&& workCb, done_cb_t&& doneCb);
  bool good() const noexcept override;
  void run() override;
  void onSuccess() noexcept override;
  void onError(const std::exception& ex) noexcept override;

  static std::unique_ptr<CallbackTask> createFromEventThread(
    EventContext*, work_cb_t&&, done_cb_t&&
  ) noexcept;
};

}}} // kclpp::async::tpool
