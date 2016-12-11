#pragma once
#include <chrono>
#include "kclpp/clock/types.h"
#include "kclpp/clock/Clock.h"

namespace kclpp_test_support {

class MockClock: public kclpp::clock::Clock {
 public:
  using NanosecondPoint = kclpp::clock::NanosecondPoint;
 protected:
  NanosecondPoint nowTime {0};
 public:
  NanosecondPoint getNow() const override;
  std::chrono::milliseconds getNowMsec() const override;
  void setNow(NanosecondPoint nextNow);
  static size_t secToNanosec(size_t numSec);
};

} // kclpp_test_support


