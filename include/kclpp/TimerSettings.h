#pragma once

#include <chrono>

namespace kclpp {

class TimerSettings {
 public:
  using time_step_t = std::chrono::milliseconds;
 protected:
  time_step_t interval_ {0};
 public:
  TimerSettings(time_step_t interval);
  TimerSettings();
  void toTimeVal(struct timeval *timeVal) const;
  struct timeval toTimeVal() const;
  time_step_t interval() const;
};

} // kclpp

