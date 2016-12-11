#include "kclpp/async/tpool/CallbackTask.h"
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

namespace kclpp { namespace async { namespace tpool {

using work_cb_t = CallbackTask::work_cb_t;
using done_outcome_t = CallbackTask::done_outcome_t;
using done_cb_t = CallbackTask::done_cb_t;
CallbackTask::CallbackTask(work_cb_t&& workCb, done_cb_t&& doneCb)
  : work_(std::forward<work_cb_t>(workCb)), onFinished_(std::forward<done_cb_t>(doneCb)){}

bool CallbackTask::good() const noexcept {
  return !!work_ && !!onFinished_;
}

void CallbackTask::run() {
  work_();
}

void CallbackTask::onSuccess() noexcept {
  onFinished_(done_outcome_t{Unit{}});
}

void CallbackTask::onError(const std::exception& ex) noexcept {
  done_outcome_t outcome {
    ThreadPoolError {ex.what()}
  };
  onFinished_(done_outcome_t{ThreadPoolError{ ex.what() }});
}

std::unique_ptr<CallbackTask> CallbackTask::createFromEventThread(EventContext *ctx,
    work_cb_t&& work, done_cb_t&& onFinished) noexcept {
  auto task = util::makeUnique<CallbackTask>(
    std::forward<work_cb_t>(work), std::forward<done_cb_t>(onFinished)
  );
  task->setSenderContext(ctx);
  task->setSenderId(std::this_thread::get_id());
  return task;
}

}}} // kclpp::async::tpool
