#pragma once
#include <atomic>
#include <functional>
#include <glog/logging.h>
#include "kclpp/UniqueToken.h"

// copied from score

namespace kclpp {

class ScopeGuard {
 public:
  using void_func_t = std::function<void()>;
 protected:
  void_func_t func_;
  UniqueToken token_;
  bool dismissed_ {false};
 public:
  ScopeGuard(void_func_t &&func);

  template<typename TCallable,
    typename std::enable_if<!std::is_same<
      typename std::remove_reference<TCallable>::type,
      void_func_t
    >::value>::type
  >
  ScopeGuard(TCallable&& callable) {
    void_func_t func = callable;
    func_ = func;
  }
  ScopeGuard(ScopeGuard&&);
  ScopeGuard& operator=(ScopeGuard&&);
  void dismiss();
  ~ScopeGuard();
};

template<typename TCallable>
ScopeGuard makeGuard(TCallable&& func) {
  return ScopeGuard{std::forward<TCallable>(func)};
}

} // kclpp
