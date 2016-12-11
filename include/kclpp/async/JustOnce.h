#pragma once

#include "kclpp/func/Function.h"

namespace kclpp { namespace async {

class JustOnce {
 public:
  using cb_t = func::Function<void>;
 protected:
  bool ran_ {false};

  cb_t func_;
 public:
  JustOnce(cb_t&& func);
  bool run();
  void operator()();
};

}} // kclpp::async

