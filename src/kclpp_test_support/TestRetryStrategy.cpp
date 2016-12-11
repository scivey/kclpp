#include "kclpp_test_support/TestRetryStrategy.h"
#include <aws/core/client/AWSError.h>
#include <aws/core/utils/UnreferencedParam.h>

namespace kclpp_test_support {

using namespace Aws;
using namespace Aws::Client;

bool TestRetryStrategy::ShouldRetry(const AWSError<CoreErrors>& error,
    long attemptedRetries) const {
  if (attemptedRetries >= maxRetries_) {
    return false;
  }
  return error.ShouldRetry();
}

long TestRetryStrategy::CalculateDelayBeforeNextRetry(const AWSError<CoreErrors>&,
    long attemptedRetries) const {
  if (attemptedRetries == 0) {
    return 0;
  }
  return scaleFactor_;
}

} // kclpp_test_support
