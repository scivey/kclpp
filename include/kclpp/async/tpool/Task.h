#pragma once
#include <thread>
#include <chrono>
#include "kclpp/func/Function.h"
#include "kclpp/util/misc.h"
#include "kclpp/MoveWrapper.h"
#include "kclpp/KCLPPError.h"
#include "kclpp/async/EventError.h"
#include "kclpp/async/queues/QueueError.h"
#include "kclpp/async/EventContext.h"
#include "kclpp/async/queues/MPMCQueue.h"
#include "kclpp/async/queues/EventDataChannel.h"
#include "kclpp/async/tpool/ThreadPoolError.h"

namespace kclpp { namespace async { namespace tpool {

class Task {
 public:
  using ctx_t = EventContext;
  using sender_id_t = std::thread::id;
 protected:
  ctx_t *senderContext_ {nullptr};
  sender_id_t senderId_ {0};
 public:
  bool hasGoodContext() const noexcept;
  bool valid() const noexcept;
  void setSenderContext(ctx_t *ctx) noexcept;
  void setSenderId(sender_id_t id) noexcept;
  sender_id_t getSenderId() const noexcept;
  ctx_t* getSenderContext() const noexcept;
  virtual bool good() const noexcept = 0;
  virtual void run() = 0;
  virtual void onSuccess() noexcept = 0;
  virtual void onError(const std::exception& ex) noexcept = 0;
  virtual ~Task() = default;
};

}}} // kclpp::async::tpool
