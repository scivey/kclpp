#pragma once

/*
  This retry strategy is adapted from Aws::Client::DefaultRetryStrategy,
  but without exponential backoff. (faster tests for certain failure cases)
*/

#include <aws/core/client/RetryStrategy.h>

namespace kclpp_test_support {

class TestRetryStrategy : public Aws::Client::RetryStrategy {
 protected:
  // I usually prefer explicitly sized ints.
  // `long` is in keeping with the `RetryStrategy` interface.
  long maxRetries_ {0};
  long scaleFactor_ {0};
 public:
  TestRetryStrategy(long maxRetries = 10, long scaleFactor = 25)
    : maxRetries_(maxRetries), scaleFactor_(scaleFactor) {}

  bool ShouldRetry(
    const Aws::Client::AWSError<Aws::Client::CoreErrors>&,
    long attemptedRetries
  ) const override;
  long CalculateDelayBeforeNextRetry(
    const Aws::Client::AWSError<Aws::Client::CoreErrors>&,
    long attemptedRetries
  ) const override;
};

} // kclpp_test_support
