#include "kclpp_test_support/MockClock.h"
#include "kclpp/clock/types.h"

namespace kclpp_test_support {

using NanosecondPoint = MockClock::NanosecondPoint;

NanosecondPoint MockClock::getNow() const {
  return nowTime;
}

std::chrono::milliseconds MockClock::getNowMsec() const {
  return std::chrono::milliseconds { nowTime.value() * 1000000 };
}

void MockClock::setNow(NanosecondPoint nextNow) {
  nowTime = nextNow;
}

size_t MockClock::secToNanosec(size_t numSec) {
  return size_t{1000000000} * numSec;
}

} // kclpp_test_support

