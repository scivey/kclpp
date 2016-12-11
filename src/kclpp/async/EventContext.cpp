#include "kclpp/async/EventContext.h"
#include "kclpp/async/EvBase.h"
#include "kclpp/async/wheel/HTimerWheel.h"
#include "kclpp/async/SignalHandlerRegistry.h"
#include "kclpp/func/Function.h"
#include "kclpp/ScopeGuard.h"
#include "kclpp/util/misc.h"

namespace kclpp { namespace async {

using queues::EventDataChannel;

using control_channel_t = typename EventContext::control_channel_t;
using base_t = typename EventContext::base_t;
using wheel_t = typename EventContext::wheel_t;
using sig_registry_t = typename EventContext::sig_registry_t;

EventContext::ControlMessage::ControlMessage(){}
EventContext::ControlMessage::ControlMessage(work_cb_t&& workFunc)
  : work(std::forward<work_cb_t>(workFunc)){}

EventContext::ControlMessage::ControlMessage(const work_cb_t& workFunc)
  : work(workFunc){}

EventContext::EventContext(){}

base_t* EventContext::getBase() {
  return base_.get();
}

wheel_t* EventContext::getWheel() {
  return wheel_.get();
}

sig_registry_t* EventContext::getSignalRegistry() {
  return sigRegistry_.get();
}

void EventContext::drainDataChannel(const std::shared_ptr<EventDataChannel>& channel) {
  EventDataChannel::Message message;
  for (;;) {
    auto readResult = channel->getQueue()->tryRead(message);
    if (!readResult.IsSuccess() || !readResult.GetResult()) {
      break;
    }
    DCHECK(!!message.work);
    message.work();
  }
}

void EventContext::drainControlChannel() {
  ControlMessage message;
  for (;;) {
    auto readResult = controlChannel_->tryRead(message);
    if (!readResult.IsSuccess() || !readResult.GetResult()) {
      break;
    }
    DCHECK(!!message.work);
    message.work();
  }
}

void EventContext::bindDataChannel(std::shared_ptr<EventDataChannel> channel) {
  EventDataChannelHandle chanHandle;
  chanHandle.channel = channel;
  chanHandle.readEvent.reset(CallbackEvent::createNewEvent(
    base_.get(), channel->getQueue()->getFDNum().GetResult(), EV_READ | EV_PERSIST
  ));
  chanHandle.readEvent->setReadHandler([this, channel]() {
    this->drainDataChannel(channel);
  });
  chanHandle.readEvent->add();
  channel->markReceiverAcked(true);
  dataChannels_.insert(std::make_pair(
    channel->getSenderID(), std::move(chanHandle)
  ));

  // the producer may have sent messages before
  // we registered the read event
  this->drainDataChannel(channel);
}

void EventContext::bindControlChannel() {
  CHECK(!controlEvent_);
  CHECK(!!controlChannel_ && !!base_);
  controlEvent_.reset(CallbackEvent::createNewEvent(
    base_.get(), controlChannel_->getFDNum().GetResult(), EV_READ | EV_PERSIST
  ));
  controlEvent_->setReadHandler([this]() {
    this->drainControlChannel();
  });
  controlEvent_->add();

  // if the EventContext was properly initialized prior to
  // being made visible to other threads, we shouldn't have any
  // existing control messages that were enqueued before we
  // registered the read event handler.
  //
  // nonetheless, draining any existing messages here seems
  // like a reasonable safeguard given that we only pay the
  // cost once (at initialization).
  this->drainControlChannel();
}

EventContext* EventContext::createNew() {
  auto ctx = new EventContext;
  auto guard = kclpp::makeGuard([ctx]() {
    delete ctx;
  });
  ctx->base_ = util::createUnique<base_t>();
  wheel_t::WheelSettings wheelSettings {
    {10, 10},
    {100, 10},
    {1000, 60},
    {60000, 60}
  };
  ctx->wheel_ = util::createUnique<wheel_t>(ctx->base_.get(), wheelSettings);
  ctx->wheel_->start();  
  ctx->sigRegistry_ = util::createUnique<sig_registry_t>(ctx->base_.get());
  ctx->controlChannel_ = util::createUnique<control_channel_t>();
  ctx->bindControlChannel();
  guard.dismiss();
  return ctx;
}

void EventContext::runSoon(const work_cb_t& work) {
  work_cb_t toRun { work };
  wheel_->addOneShot(std::move(toRun), std::chrono::milliseconds{5});
}

using send_outcome_t = EventContext::send_outcome_t;

send_outcome_t EventContext::threadsafeTrySendControlMessage(ControlMessage&& message) {
  return controlChannel_->tryLockAndSend(std::forward<ControlMessage>(message));
}

send_outcome_t EventContext::threadsafeRegisterDataChannel(
    std::shared_ptr<EventDataChannel> dataChannel) {
  return controlChannel_->tryLockAndSend(ControlMessage{[dataChannel, this]() {
    this->bindDataChannel(dataChannel);
  }});
}



}} // kclpp::async
