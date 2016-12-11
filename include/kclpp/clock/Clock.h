#pragma once
#include <chrono>
#include "kclpp/clock/types.h"

namespace kclpp { namespace clock {

class Clock {
 public:
  virtual NanosecondPoint getNow() const = 0;
  virtual std::chrono::milliseconds getNowMsec() const = 0;
  virtual ~Clock() = default;
};

}} // kclpp::clock