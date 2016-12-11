#pragma once
#include <chrono>
#include "kclpp/clock/Clock.h"
#include "kclpp/clock/types.h"

namespace kclpp { namespace clock {

class SystemClock: public Clock {
 public:
  NanosecondPoint getNow() const override;
  std::chrono::milliseconds getNowMsec() const override;
};

}} // kclpp::clock