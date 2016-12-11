#pragma once
#include "kclpp/async/EventContext.h"
#include "kclpp/async/tpool/ThreadPool.h"
#include "kclpp/util/misc.h"

namespace kclpp { namespace async {


class KCLAsyncContext {
 public:
  using ThreadPool = tpool::ThreadPool;
 protected:
  struct InnerState {
    std::unique_ptr<EventContext> eventContext {nullptr};
    std::unique_ptr<ThreadPool> threadPool {nullptr};  
  };
  InnerState state_;
  KCLAsyncContext(InnerState&& state);
 public:
  static KCLAsyncContext* createNew(size_t nThreads = 4);
  EventContext* getEventContext() const;
  ThreadPool* getThreadPool() const;
  using work_cb_t = EventContext::work_cb_t;
  void runInEventThread(work_cb_t work);
  void runSoonInEventThread(work_cb_t work);
  ~KCLAsyncContext();
};


}} // kclpp::async
