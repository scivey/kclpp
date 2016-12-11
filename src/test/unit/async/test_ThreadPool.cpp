#include <gtest/gtest.h>
#include <thread>
#include <atomic>
#include <chrono>
#include "kclpp/async/EventContext.h"
#include "kclpp/async/tpool/ThreadPool.h"
#include "kclpp/async/tpool/CallbackTask.h"

using namespace kclpp;
using namespace kclpp::async;
using namespace kclpp::async::tpool;
using kclpp::func::Function;
using namespace std;

TEST(TestThreadPool, TestSanity1) {
  auto evCtx = util::createShared<EventContext>();
  std::atomic<bool> workRan {false};
  std::atomic<bool> doneCallbackRan {false};
  auto pool = util::createShared<ThreadPool>(size_t{4});
  auto task = CallbackTask::createFromEventThread(evCtx.get(),
    [&workRan]() mutable {
      workRan.store(true);
    },
    [&doneCallbackRan](const CallbackTask::done_outcome_t& outcome) mutable {
      EXPECT_TRUE(outcome.IsSuccess());
      doneCallbackRan.store(true);
    }
  );
  EXPECT_TRUE(pool->start().IsSuccess());
  EXPECT_TRUE(pool->trySubmit(std::move(task)).IsSuccess());
  bool didWorkRun {false};
  bool didCallbackRun {false};
  for (;;) {
    auto workResult = workRan.load();
    if (workResult != didWorkRun) {
      didWorkRun = workResult;
    }
    auto doneResult = doneCallbackRan.load();
    if (doneResult != didCallbackRun) {
      didCallbackRun = doneResult;
    }
    if (didWorkRun && didCallbackRun) {
      break;
    }
    evCtx->getBase()->runOnce();
  }
  EXPECT_TRUE(workRan.load());
  EXPECT_TRUE(doneCallbackRan.load());
}

