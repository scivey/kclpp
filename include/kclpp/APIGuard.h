#pragma once
#include <stdexcept>
#include <atomic>
#include <aws/core/Aws.h>

namespace kclpp {

class APIGuard;

class APIGuardFactory {
 public:
  class AlreadyBuilt : public std::runtime_error {
   public:
    AlreadyBuilt();
  };

 protected:
  Aws::SDKOptions options_;
  std::atomic<bool> built_ {false};
  APIGuardFactory(const APIGuardFactory&) = delete;
  APIGuardFactory& operator=(const APIGuardFactory&) = delete;
 public:
  APIGuardFactory();
  APIGuardFactory(Aws::SDKOptions &&options);

  Aws::SDKOptions& getOptions();
  const Aws::SDKOptions& getOptions() const;
  APIGuard build();
};

class APIGuard {
 protected:
  Aws::SDKOptions options_;
  std::atomic<bool> active_ {false};
  APIGuard(const APIGuard&) = delete;
  APIGuard& operator=(const APIGuard&) = delete;
  friend class APIGuardFactory;
  APIGuard();
  APIGuard(Aws::SDKOptions &&options);
  void init();
 public:
  APIGuard(APIGuard &&other);
  APIGuard& operator=(APIGuard &&other);
  ~APIGuard();
};

} // kclpp
