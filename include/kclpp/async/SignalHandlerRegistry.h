#pragma once

#include <vector>
#include <memory>
#include <glog/logging.h>

#include "kclpp/async/EvBase.h"
#include "kclpp/async/SignalEvent.h"

namespace kclpp { namespace async {

class SignalHandlerRegistry {
 public:
  using cb_t = typename SignalEvent::cb_t;
 protected:
  EvBase *base_ {nullptr};
  std::vector<std::shared_ptr<SignalEvent>> handlers_;
 public:
  SignalHandlerRegistry(EvBase *base);

  void addHandler(int signo, const cb_t& callable);
  void addHandler(int signo, cb_t&& callable);

  static SignalHandlerRegistry* createNew(EvBase *base);

  template<typename TCallable,
    typename = typename std::enable_if<!std::is_same<TCallable, cb_t>::value, TCallable>::type>
  void addHandler(int signo, TCallable&& callable) {
    cb_t callback = callable;
    addHandler(signo, callback);
  }
};


}} // kclpp::async