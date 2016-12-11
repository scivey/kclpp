#include "kclpp/APIGuard.h"
#include <glog/logging.h>


namespace kclpp {

class APIGuard;

APIGuardFactory::AlreadyBuilt::AlreadyBuilt(): std::runtime_error("AlreadyBuilt") {}

APIGuardFactory::APIGuardFactory(){}

APIGuardFactory::APIGuardFactory(Aws::SDKOptions &&options)
  : options_(std::forward<Aws::SDKOptions>(options)) {}

Aws::SDKOptions& APIGuardFactory::getOptions() {
  return options_;
}

const Aws::SDKOptions& APIGuardFactory::getOptions() const {
  return options_;
}

APIGuard APIGuardFactory::build() {
  APIGuard guard {std::move(options_)};
  guard.init();
  return guard;
}

APIGuard::APIGuard(){}

APIGuard::APIGuard(Aws::SDKOptions &&options)
  : options_(std::forward<Aws::SDKOptions>(options)){}

void APIGuard::init() {
  bool expected = false;
  bool desired = true;
  if (active_.compare_exchange_strong(expected, desired)) {
    Aws::InitAPI(options_);
  }
}

APIGuard::APIGuard(APIGuard &&other): options_(other.options_) {
  bool otherActive = other.active_.load();
  if (otherActive) {
    bool desired = false;
    bool expected = true;
    if (other.active_.compare_exchange_strong(expected, desired)) {
      active_.store(true);
    }
  }
}

APIGuard& APIGuard::operator=(APIGuard &&other) {
  std::swap(options_, other.options_);
  bool otherActive = other.active_.load();
  bool amActive = active_.load();
  active_.store(otherActive);
  other.active_.store(amActive);
  return *this;
}

APIGuard::~APIGuard() {
  if (active_.load()) {
    bool expected = true;
    bool desired = false;
    if (active_.compare_exchange_strong(expected, desired)) {
      LOG(INFO) << "APIGuard : shutting down AWS API.";
      Aws::ShutdownAPI(options_);
    }
  }
}


} // kclpp
