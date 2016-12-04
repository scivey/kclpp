#pragma once
#include <stdexcept>
#include <atomic>

#include <glog/logging.h>
#include <aws/core/utils/DateTime.h>
#include <aws/core/Aws.h>


namespace kclpp {

class APIGuard;

class APIGuardFactory {
 public:
  class AlreadyBuilt : public std::runtime_error {
   public:
    AlreadyBuilt(): std::runtime_error("AlreadyBuilt") {}
  };

 protected:
  Aws::SDKOptions options_;
  std::atomic<bool> built_ {false};
  APIGuardFactory(const APIGuardFactory&) = delete;
  APIGuardFactory& operator=(const APIGuardFactory&) = delete;
 public:
  APIGuardFactory(){}
  APIGuardFactory(Aws::SDKOptions &&options)
    : options_(std::forward<Aws::SDKOptions>(options)) {}

  Aws::SDKOptions& getOptions() {
    return options_;
  }
  const Aws::SDKOptions& getOptions() const {
    return options_;
  }
  APIGuard build();
};

class APIGuard {
 protected:
  Aws::SDKOptions options_;
  std::atomic<bool> active_ {false};
  APIGuard(const APIGuard&) = delete;
  APIGuard& operator=(const APIGuard&) = delete;
  friend class APIGuardFactory;
  APIGuard(){}
  APIGuard(Aws::SDKOptions &&options)
    : options_(std::forward<Aws::SDKOptions>(options)){}
  void init() {
    bool expected = false;
    bool desired = true;
    if (active_.compare_exchange_strong(expected, desired)) {
      Aws::InitAPI(options_);
    }
  }
 public:
  APIGuard(APIGuard &&other): options_(other.options_) {
    bool otherActive = other.active_.load();
    if (otherActive) {
      bool desired = false;
      bool expected = true;
      if (other.active_.compare_exchange_strong(expected, desired)) {
        active_.store(true);
      }
    }
  }
  APIGuard& operator=(APIGuard &&other) {
    std::swap(options_, other.options_);
    bool otherActive = other.active_.load();
    bool amActive = active_.load();
    active_.store(otherActive);
    other.active_.store(amActive);
    return *this;
  }
  ~APIGuard() {
    if (active_.load()) {
      bool expected = true;
      bool desired = false;
      if (active_.compare_exchange_strong(expected, desired)) {
        LOG(INFO) << "APIGuard : shutting down AWS API.";
        Aws::ShutdownAPI(options_);
      }
    }
  }
};

APIGuard APIGuardFactory::build() {
  APIGuard guard {std::move(options_)};
  guard.init();
  return guard;
}


} // kclpp
