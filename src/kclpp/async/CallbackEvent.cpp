#include "kclpp/async/CallbackEvent.h"
#include <glog/logging.h>

namespace kclpp { namespace async {

void CallbackEvent::onReadable() noexcept {
  if (readCallback_) {
    readCallback_();
  } else {
    LOG(INFO) << "onReadable: no readCallback registered.";
  }
}
void CallbackEvent::onWritable() noexcept {
  if (writeCallback_) {
    writeCallback_();
  } else {
    LOG(INFO) << "onWritable: no writeCallback registered.";
  }
}
void CallbackEvent::onTimeout() noexcept {
  if (timeoutCallback_) {
    timeoutCallback_();
  } else {
    LOG(INFO) << "onTimeout: no timeoutCallback registered.";
  }
}

void CallbackEvent::onSignal() noexcept {}

}} // kclpp::async
