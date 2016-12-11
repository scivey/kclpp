#pragma once

#include <glog/logging.h>
#include "kclpp/async/BaseEvent.h"
#include "kclpp/func/Function.h"

namespace kclpp { namespace async {

class SignalEvent: public BaseEvent<SignalEvent> {
 public:
  using cb_t = func::Function<void>;
 protected:
  cb_t callback_;
 public:
  void onSignal() const noexcept {
    CHECK(!!callback_);
    callback_();
  }
  template<typename TCallable>
  void setHandler(TCallable&& callable) noexcept {
    callback_ = std::forward<TCallable>(callable);
  }
  void onReadable() noexcept {
    DCHECK(false);
  }
  void onWritable() noexcept {
    DCHECK(false);    
  }
  void onTimeout() noexcept {
    DCHECK(false);    
  }
};

}} // kclpp::async
