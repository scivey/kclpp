#pragma once
#include <functional>
#include <event2/event.h>
#include <glog/logging.h>
#include "kclpp/util/misc.h"
#include "kclpp/TimerSettings.h"
#include "kclpp/async/EvBase.h"
#include "kclpp/async/EventError.h"
#include "kclpp/Outcome.h"
#include "kclpp/ScopeGuard.h"

namespace kclpp { namespace async {


template<typename T>
class BaseEvent {
 public:
  using event_t = struct event;
  using event_ptr_t = typename kclpp::util::unique_destructor_ptr<event_t>::type;
  using fd_t = evutil_socket_t;
 protected:
  event_ptr_t event_ {nullptr};
  T* getThis() const noexcept {
    return (T*) this;
  }
 public:
  bool good() const noexcept {
    return !!event_;
  }
  operator bool() const noexcept {
    return good();
  }
  event_t* getEvent() const noexcept {
    DCHECK(good());
    return event_.get();
  }
  using get_outcome_t = Outcome<fd_t, EventError>;
  get_outcome_t getFD() const noexcept {
    if (!good()) {
      return get_outcome_t {
        EventError {"Invalid event."}
      };
    }
    fd_t result = event_get_fd(event_.get());
    if (result < 0) {
      return get_outcome_t {
        EventError {"Valid event, but invalid file descriptor."}
      };
    }
    return get_outcome_t {result};
  }
  static T* createNewEvent(EvBase *base, fd_t fd, short what) noexcept {
    T* result {nullptr};
    auto guard = makeGuard([&result]() {
      if (result) {
        delete result;
        result = nullptr;
      }
    });
    result = new T;
    result->event_ = kclpp::util::asDestructorPtr<event_t>(
      event_new(base->getBase(), fd, what, T::libeventCallback, (void*) result),
      event_free
    );
    guard.dismiss();
    return result;
  }
  static T* createNewSignalEvent(EvBase *base, int signum) noexcept {
    return createNewEvent(
      base, signum, EV_SIGNAL|EV_PERSIST
    );
  }
  static T* createNewTimerEvent(EvBase *base, const TimerSettings &settings) noexcept {
    auto evt = createNewEvent(
      base, -1, EV_PERSIST
    );
    evt->add(settings);
    return evt;
  }
  void doOnReadable() noexcept {
    getThis()->onReadable();
  }
  void doOnWritable() noexcept {
    getThis()->onWritable();
  }
  void doOnSignal() noexcept {
    getThis()->onSignal();
  }
  void doOnTimeout() noexcept {
    getThis()->onTimeout();
  }
  static void libeventCallback(evutil_socket_t sockFd, short what, void *arg) noexcept {
    auto ctx = (T*) arg;
    if (what & EV_READ) {
      ctx->doOnReadable();
    }
    if (what & EV_WRITE) {
      ctx->doOnWritable();
    }
    if (what & EV_SIGNAL) {
      ctx->doOnSignal();
    }
    if (what & EV_TIMEOUT) {
      ctx->doOnTimeout();
    }
  }
  void add() noexcept {
    DCHECK(!!event_);
    event_add(event_.get(), nullptr);
  }
  void add(const TimerSettings &timeout) noexcept {
    timeval timeVal = timeout.toTimeVal();
    add(&timeVal);
  }
  void add(timeval *timeout) noexcept {
    DCHECK(!!event_);
    event_add(event_.get(), timeout);
  }
  void del() noexcept {
    DCHECK(good());
    DCHECK(event_del(event_.get()) == 0);
  }
};

}} // kclpp::async
