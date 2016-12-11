#pragma once
#include <sstream>
#include <string>
#include <cstdio>
#include <glog/logging.h>
#include "kclpp/async/BaseEvent.h"
#include "kclpp/func/Function.h"

namespace kclpp { namespace async {

class CallbackEvent: public BaseEvent<CallbackEvent> {
 public:
  using string_t = std::string;
  using event_cb_t = kclpp::func::Function<void>;

 protected:
  event_cb_t readCallback_;
  event_cb_t writeCallback_;
  event_cb_t timeoutCallback_;

 public:
  void onReadable() noexcept;
  void onWritable() noexcept;
  void onTimeout() noexcept;
  void onSignal() noexcept;

  template<typename TCallable>
  void setReadHandler(TCallable&& callable) {
    readCallback_ = std::forward<TCallable>(callable);
  }
  template<typename TCallable>
  void setReadHandler(const TCallable& callable) {
    readCallback_ = callable;
  }
  template<typename TCallable>
  void setWriteHandler(TCallable&& callable) {
    writeCallback_ = std::forward<TCallable>(callable);
  }
  template<typename TCallable>
  void setWriteHandler(const TCallable& callable) {
    writeCallback_ = callable;
  }
  template<typename TCallable>
  void setTimeoutHandler(TCallable&& callable) {
    timeoutCallback_ = std::forward<TCallable>(callable);
  }
  template<typename TCallable>
  void setTimeoutHandler(const TCallable& callable) {
    timeoutCallback_ = callable;
  }
};


}} // kclpp::async
