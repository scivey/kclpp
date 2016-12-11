#include "kclpp/async/KCLAsyncContext.h"
#include "kclpp/async/EventContext.h"
#include "kclpp/async/tpool/ThreadPool.h"
#include "kclpp/util/misc.h"
#include "kclpp/TimeLogger.h"

namespace kclpp { namespace async {

using kclpp::async::tpool::ThreadPool;

KCLAsyncContext::KCLAsyncContext(InnerState&& state): state_(std::move(state)) {}

KCLAsyncContext* KCLAsyncContext::createNew(size_t nThreads) {
  InnerState innerState;
  innerState.eventContext = util::createUnique<EventContext>();
  innerState.threadPool = util::createUnique<ThreadPool>(nThreads);
  return new KCLAsyncContext(std::move(innerState));
}

EventContext*  KCLAsyncContext::getEventContext() const {
  auto ctx = state_.eventContext.get();
  DCHECK(!!ctx);
  return ctx;
}
ThreadPool* KCLAsyncContext::getThreadPool() const {
  auto pool = state_.threadPool.get();
  DCHECK(!!pool);
  return pool;
}

using work_cb_t = EventContext::work_cb_t;

void KCLAsyncContext::runInEventThread(work_cb_t work) {
  auto outcome = getEventContext()->threadsafeTrySendControlMessage(EventContext::ControlMessage{work});
  DCHECK(outcome.IsSuccess());
}

void KCLAsyncContext::runSoonInEventThread(work_cb_t work) {
  runInEventThread([this, work]() {
    getEventContext()->runSoon(work);
  });
}

KCLAsyncContext::~KCLAsyncContext() {
  state_.threadPool->stopJoin();
  state_.threadPool.reset();
  state_.eventContext.reset();
}


}} // kclpp::async
