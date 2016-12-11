#include "kclpp/async/TimerEvent.h"
#include <glog/logging.h>
#include "kclpp/async/BaseEvent.h"
#include "kclpp/func/Function.h"

namespace kclpp { namespace async {

void TimerEvent::onTimeout() noexcept {
  DCHECK(!!callback_);
  if (callback_) {
    callback_();
  }
}

void TimerEvent::onReadable() noexcept {}
void TimerEvent::onWritable() noexcept {}
void TimerEvent::onSignal() noexcept {}

TimerEvent* TimerEvent::createNew(EvBase *base, bool persist) noexcept {
  int flags = 0;
  if (persist) {
    flags |= EV_PERSIST;
  }
  return createNewEvent(
    base, -1, flags
  );
}

bool TimerEvent::isPending() const noexcept {
  if (!good()) {
    return false;
  }
  return event_pending(getEvent(), EV_TIMEOUT, nullptr);
}


}} // kclpp::async
