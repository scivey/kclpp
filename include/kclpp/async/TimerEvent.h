#pragma once

#include "kclpp/async/BaseEvent.h"
#include "kclpp/func/Function.h"

namespace kclpp { namespace async {


class TimerEvent: public BaseEvent<TimerEvent> {
 protected:
  func::Function<void> callback_;
 public:
  template<typename TCallable>
  void setHandler(TCallable&& callable) noexcept {
    callback_ = std::forward<TCallable>(callable);
  }
  void onReadable() noexcept;
  void onWritable() noexcept;
  void onSignal() noexcept;
  void onTimeout() noexcept;
  static TimerEvent* createNew(EvBase *base, bool persist = true) noexcept;
  bool isPending() const noexcept;
};


}} // kclpp::async
