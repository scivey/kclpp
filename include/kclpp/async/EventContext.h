#pragma once

#include <unordered_map>
#include "kclpp/Unit.h"
#include "kclpp/func/Function.h"
#include "kclpp/util/misc.h"
#include "kclpp/async/SignalHandlerRegistry.h"
#include "kclpp/async/queues/MPSCEventChannel.h"
#include "kclpp/async/queues/EventDataChannel.h"
#include "kclpp/async/wheel/HTimerWheel.h"
#include "kclpp/async/CallbackEvent.h"
#include "kclpp/Outcome.h"

namespace kclpp { namespace async {

class EvBase;

class EventContext {
 public:
  using base_t = EvBase;
  using base_ptr_t = std::unique_ptr<base_t>;
  using wheel_t = wheel::HTimerWheel;
  using wheel_ptr_t = std::unique_ptr<wheel_t>;
  using sig_registry_t = SignalHandlerRegistry;
  using sig_registry_ptr_t = std::unique_ptr<SignalHandlerRegistry>;
  using work_cb_t = func::Function<void>;
  using EventDataChannel = queues::EventDataChannel;

  struct ControlMessage {
    work_cb_t work;
    ControlMessage(work_cb_t&&);
    ControlMessage(const work_cb_t&);
    ControlMessage();
  };
  using control_channel_t = queues::MPSCEventChannel<ControlMessage>;
  using control_channel_ptr_t = std::unique_ptr<control_channel_t>;

  using data_channel_t = EventDataChannel;
  using data_channel_ptr_t = std::shared_ptr<data_channel_t>;

  struct EventDataChannelHandle {
    std::unique_ptr<CallbackEvent> readEvent {nullptr};
    data_channel_ptr_t channel {nullptr};
  };

 protected:
  base_ptr_t base_ {nullptr};
  wheel_ptr_t wheel_ {nullptr};  
  sig_registry_ptr_t sigRegistry_ {nullptr};
  control_channel_ptr_t controlChannel_ {nullptr};
  std::unique_ptr<CallbackEvent> controlEvent_ {nullptr};
  using data_channel_map_t = std::unordered_map<std::thread::id, EventDataChannelHandle>;
  data_channel_map_t dataChannels_;

  EventContext();
  void drainControlChannel();
  void bindControlChannel();
  void drainDataChannel(const data_channel_ptr_t&);
  void bindDataChannel(data_channel_ptr_t);
 public:
  base_t* getBase();
  wheel_t* getWheel();  
  sig_registry_t* getSignalRegistry();
  static EventContext* createNew();
  void runSoon(const work_cb_t& func);
  using send_outcome_t = Outcome<Unit, queues::QueueError>;
  send_outcome_t threadsafeTrySendControlMessage(ControlMessage&& msg);
  send_outcome_t threadsafeRegisterDataChannel(std::shared_ptr<EventDataChannel>);
};

}} // kclpp::async
