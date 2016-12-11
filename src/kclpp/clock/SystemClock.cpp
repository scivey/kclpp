#include "kclpp/clock/SystemClock.h"

namespace kclpp { namespace clock {

NanosecondPoint SystemClock::getNow() const {
  auto epochTime = std::chrono::system_clock::now().time_since_epoch();
  long result = epochTime / std::chrono::nanoseconds{1};
  return NanosecondPoint { (size_t) result };
}

std::chrono::milliseconds SystemClock::getNowMsec() const {
  auto epochTime = std::chrono::system_clock::now().time_since_epoch();
  long result = epochTime / std::chrono::milliseconds{1};
  return std::chrono::milliseconds { (size_t) result};
}



}} // kclpp::clock
