#pragma once
#include <mutex>
#include "kclpp/KCLPPError.h"
#include "kclpp/async/EventFD.h"
#include "kclpp/async/queues/SPSCQueue.h"
#include "kclpp/async/queues/QueueError.h"

namespace kclpp { namespace async { namespace queues {


template<typename T>
class SPSCEventChannel {
 public:
  using queue_t = SPSCQueue<T>;
  using queue_ptr_t = std::unique_ptr<queue_t>;
 protected:
  EventFD eventFD_;
  queue_ptr_t queue_ {nullptr};

 public:
  SPSCEventChannel(EventFD&& efd, queue_ptr_t&& spQueue)
    : eventFD_(std::move(efd)), queue_(std::move(spQueue)) {}

  bool good() const {
    return !!eventFD_ && !!queue_;
  }
  explicit operator bool() const {
    return good();
  }
  static SPSCEventChannel create() {
    return SPSCEventChannel(
      std::move(EventFD::create().GetResult().value()),
      util::makeUnique<queue_t>(size_t {10000})
    );
  }

  static SPSCEventChannel* createNew() {
    return new SPSCEventChannel{create()};
  }

  using try_send_outcome_t = Outcome<Unit, QueueError>; 
  try_send_outcome_t trySend(T&& msg) {
    DCHECK(good());
    if (queue_->try_enqueue(std::forward<T>(msg))) {
      auto writeRes = eventFD_.write(1);
      if (!writeRes.IsSuccess()) {
        return try_send_outcome_t {
          PartialWriteError{"Enqueued message, but could not signal EventFD."}
        };
      }
      return try_send_outcome_t{Unit{}};
    }
    return try_send_outcome_t {
      QueueFull {"Queue is full."}
    };
  }

  using try_read_outcome_t = Outcome<bool, EventError>;
  try_read_outcome_t tryRead(T& result) {
    DCHECK(good());
    auto readRes = eventFD_.read();
    if (!readRes.IsSuccess()) {
      // FIXME: handle NotReady events.
      return try_read_outcome_t {
        readRes.GetError()
      };
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