#pragma once
#include <string>
#include "kclpp_test_support/KinesisClientTestContext.h"
#include "kclpp/UniqueToken.h"


namespace kclpp_test_support {

struct StreamGuardOptions {
  std::string streamName {"some-stream"};
  size_t numShards {10};
  StreamGuardOptions& withName(const std::string& name);
  StreamGuardOptions& withNumShards(size_t n);
};

class StreamGuard {
 protected:
  kclpp::UniqueToken token_;
  KinesisClientTestContext *context_ {nullptr};
  StreamGuardOptions options_;
  StreamGuard(kclpp::UniqueToken&& token,
    KinesisClientTestContext* ctx,
    const StreamGuardOptions& options
  );
 public:
  StreamGuard(StreamGuard&& other);
  StreamGuard& operator=(StreamGuard&& other);
  static StreamGuard create(KinesisClientTestContext *ctx, const StreamGuardOptions& options);
  ~StreamGuard();
};

} // kclpp_test_support
