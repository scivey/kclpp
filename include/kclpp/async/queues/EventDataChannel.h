#pragma once
#include <thread>
#include <glog/logging.h>
#include "kclpp/async/queues/SPSCEventChannel.h"
#include "kclpp/func/Function.h"
#include "kclpp/util/misc.h"

namespace kclpp { namespace async { namespace queues {

class EventDataChannel {
 public:
  using work_cb_t = func::Function<void>;
  struct Message {
    work_cb_t work;
    Message(work_cb_t&& func): work(std::forward<work_cb_t>(func)){}
    Message(){}
  };
  using queue_t = SPSCEventChannel<Message>;
  using queue_ptr_t = std::shared_ptr<queue_t>;
 protected:
  std::thread::id senderID_ {0};
  queue_ptr_t queue_ {nullptr};
  std::atomic<bool> receiverAcked_ {false};
 public:
  void markReceiverAcked(bool isListening) {
    receiverAcked_.store(isListening);
  }
  bool hasReceiverAcked() const {
    return receiverAcked_.load();
  }
  EventDataChannel(std::thread::id tid, queue_ptr_t queuePtr)
    : senderID_(tid), queue_(queuePtr) {}
  queue_t* getQueue() {
    DCHECK(!!queue_);
    return queue_.get();
  }
  std::thread::id getSenderID() const {
    return senderID_;
  }
  static std::shared_ptr<EventDataChannel> createSharedAsSender() {
    auto tid = std::this_thread::get_id();
    auto workQueue = util::createShared<queue_t>();
    return std::shared_ptr<EventDataChannel>{
      new EventDataChannel {tid, workQueue}
    };
  }
};

}}}