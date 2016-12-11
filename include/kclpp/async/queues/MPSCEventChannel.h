#pragma once
#include <mutex>
#include "kclpp/locks/SpinLock.h"
#include "kclpp/KCLPPError.h"
#include "kclpp/async/EventFD.h"
#include "kclpp/async/queues/SPSCQueue.h"
#include "kclpp/async/queues/QueueError.h"

namespace kclpp { namespace async { namespace queues {


template<typename T>
class MPSCEventChannel {
 public:
  using queue_t = SPSCQueue<T>;
  using queue_ptr_t = std::unique_ptr<queue_t>;
  using lock_t = kclpp::locks::SpinLock;
 protected:
  EventFD eventFD_;
  queue_ptr_t queue_ {nullptr};
  lock_t lock_;

 public:
  MPSCEventChannel(EventFD&& efd, queue_ptr_t&& spQueue, lock_t&& lock)
    : eventFD_(std::move(efd)), queue_(std::move(spQueue)), lock_(std::move(lock)){}

  bool good() const {
    return !!eventFD_ && !!queue_ && !!lock_;
  }
  explicit operator bool() const {
    return good();
  }
  static MPSCEventChannel create() {
    return MPSCEventChannel(
      std::move(EventFD::create().GetResult().value()),
      util::makeUnique<queue_t>(size_t {20000}),
      std::move(lock_t::create().GetResult().value())
    );
  }

  static MPSCEventChannel* createNew() {
    return new MPSCEventChannel{create()};
  }

  using try_send_outcome_t = Outcome<Unit, QueueError>;
  try_send_outcome_t tryLockAndSend(T&& msg) {
    DCHECK(good());
    std::lock_guard<lock_t> guard {lock_};
    if (queue_->try_enqueue(std::forward<T>(msg))) {
      auto writeRes = eventFD_.write(1);
      if (!writeRes.IsSuccess()) {
        return try_send_outcome_t {
          PartialWriteError("Enqueued message, but could not signal EventFD.")
        };
      }
      return try_send_outcome_t{ Unit{} };
    }
    return try_send_outcome_t{ QueueFull{"Queue is full."} };
  }

  using try_read_outcome_t = Outcome<bool, EventError>;
  try_read_outcome_t tryRead(T& result) {
    DCHECK(good());
    auto readRes = eventFD_.read();
    // if (readRes.hasException() && !readRes.template hasException<EventFD::NotReady>()) {
    if (!readRes.IsSuccess()) {
      const std::string& errMsg = readRes.GetError().what();
      if (errMsg.find("NotReady") != 0) {
        return try_read_outcome_t {
          readRes.GetError()
        };
      }
    }
    return try_read_outcome_t {
      queue_->try_dequeue(result)
    };
  }

  auto getFDNum() -> decltype(std::declval<EventFD>().getFDNum()) {
    return eventFD_.getFDNum();
  }
};

}}} // kclpp::async::queues