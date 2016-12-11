#include <thread>
#include <chrono>
#include "kclpp/async/tpool/Task.h"
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

using ctx_t = Task::ctx_t;
using sender_id_t = Task::sender_id_t;


bool Task::hasGoodContext() const noexcept {
  return !!senderContext_ && senderId_ > sender_id_t{0};
}

bool Task::valid() const noexcept {
  return hasGoodContext() && good();
}

void Task::setSenderContext(ctx_t *ctx) noexcept {
  senderContext_ = ctx;
}

void Task::setSenderId(sender_id_t id) noexcept {
  senderId_ = id;
}

sender_id_t Task::getSenderId() const noexcept {
  return senderId_;
}

ctx_t* Task::getSenderContext() const noexcept {
  return senderContext_;
}

}}} // kclpp::async::tpool
