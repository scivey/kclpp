#pragma once
#include <chrono>
#include <string>
#include "kclpp/UniqueToken.h"

namespace kclpp {

class TimeLogger {
 protected:
  using sys_time_t = decltype(std::chrono::system_clock::now());
  std::string name_;
  UniqueToken token_;
  sys_time_t startTime_;
  TimeLogger(std::string&& name, UniqueToken&& token, sys_time_t startTime);

 public:
  TimeLogger(TimeLogger&& other);
  TimeLogger& operator=(TimeLogger&& other);
  static TimeLogger create(std::string&& timerName);
  static TimeLogger create();
  ~TimeLogger();
};


} // kclpp
