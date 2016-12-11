#include "kclpp/TimerSettings.h"

namespace kclpp {

TimerSettings::TimerSettings(){}
TimerSettings::TimerSettings(typename TimerSettings::time_step_t interval)
  : interval_(interval) {}

void TimerSettings::toTimeVal(struct timeval *timeVal) const {
  int64_t msec = interval_.count();
  timeVal->tv_sec = msec / 1000;
  timeVal->tv_usec = (msec % 1000) * 1000;
}

struct timeval TimerSettings::toTimeVal() const {
  struct timeval timeVal;
  toTimeVal(&timeVal);
  return timeVal;
}

typename TimerSettings::time_step_t TimerSettings::interval() const {
  return interval_;
}

} // kclpp
