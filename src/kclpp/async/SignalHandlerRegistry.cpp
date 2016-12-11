#include "kclpp/async/SignalHandlerRegistry.h"
#include <glog/logging.h>

namespace kclpp { namespace async {

using cb_t = typename SignalHandlerRegistry::cb_t;

SignalHandlerRegistry::SignalHandlerRegistry(EvBase *base): base_(base) {}

void SignalHandlerRegistry::addHandler(int signo, const cb_t& callable) {
  DCHECK(!!base_);
  std::shared_ptr<SignalEvent> evt {
    SignalEvent::createNewSignalEvent(base_, signo)
  };
  evt->setHandler(callable);
  evt->add();
  handlers_.push_back(std::move(evt));
}

void SignalHandlerRegistry::addHandler(int signo, cb_t&& callable) {
  DCHECK(!!base_);
  std::shared_ptr<SignalEvent> evt {
    SignalEvent::createNewSignalEvent(base_, signo)
  };
  evt->setHandler(std::forward<cb_t>(callable));
  evt->add();
  handlers_.push_back(std::move(evt));
}

SignalHandlerRegistry* SignalHandlerRegistry::createNew(EvBase *base) {
  return new SignalHandlerRegistry(base);
}


}} // kclpp::async