#include "kclpp/TimeLogger.h"
#include <glog/logging.h>
using namespace std;

namespace kclpp {

TimeLogger::TimeLogger(std::string&& name, UniqueToken&& token, sys_time_t startTime)
: name_(std::forward<std::string>(name)),
  token_(std::move(token)),
  startTime_(startTime) {}

TimeLogger::TimeLogger(TimeLogger&& other)
: name_(std::move(other.name_)),
  token_(std::move(other.token_)),
  startTime_(other.startTime_) {}

TimeLogger& TimeLogger::operator=(TimeLogger&& other) {
  std::swap(name_, other.name_);
  std::swap(token_, other.token_);
  std::swap(startTime_, other.startTime_);
  return *this;
}

TimeLogger TimeLogger::create(std::string&& timerName) {
  UniqueToken token;
  token.mark();
  auto start = std::chrono::system_clock::now();
  return TimeLogger(
    std::forward<std::string>(timerName),
    std::move(token),
    start
  );
}

TimeLogger TimeLogger::create() {
  return create("DefaultTimerName");
}

TimeLogger::~TimeLogger() {
  if (token_) {
    token_.clear();
    auto endTime = std::chrono::system_clock::now();
    auto duration = endTime - startTime_;
    size_t nsec = duration.count();
    double ns = nsec;
    ns /= 1000000.0;
    LOG(INFO) << "TimeLogger['" << name_ << "'] : " << ns;
  }
}


} // kclpp
