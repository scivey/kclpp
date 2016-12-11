#pragma once
#include <string>
#include "kclpp/async/KCLAsyncContext.h"
#include "kclpp/kinesis/KCLKinesisClient.h"
#include "kclpp/locks/ThreadBaton.h"

namespace kclpp_test_support {


struct KinesisClientTestContext {
  using KCLAsyncContext = kclpp::async::KCLAsyncContext;
  std::shared_ptr<KCLAsyncContext> asyncContext {nullptr};
  std::shared_ptr<kclpp::kinesis::KCLKinesisClient> client {nullptr};
  KinesisClientTestContext(std::shared_ptr<KCLAsyncContext> asyncCtx);
  KinesisClientTestContext();

  ~KinesisClientTestContext();
  void loopOnBaton(kclpp::locks::ThreadBaton& baton);
  void waitUntilStreamIsActive(const std::string& streamName);
  void waitUntilStreamDoesNotExist(const std::string& streamName);
};

} // kclpp_test_support
