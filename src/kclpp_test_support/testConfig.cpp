#include "kclpp_test_support/testConfig.h"
#include "kclpp_test_support/TestRetryStrategy.h"
#include <aws/core/utils/threading/Executor.h>

using Aws::Utils::Threading::PooledThreadExecutor;
using Aws::Client::ClientConfiguration;
namespace kclpp_test_support {

Aws::Client::ClientConfiguration makeClientConfig() {
  ClientConfiguration config;
  config.executor = Aws::MakeShared<PooledThreadExecutor>("pool", 4);
  {
    long numRetries {3};
    long scaleFactor {5};
    config.retryStrategy = std::make_shared<TestRetryStrategy>(
      numRetries, scaleFactor
    );
  }
  config.scheme = Aws::Http::Scheme::HTTP;
  config.verifySSL = false;
  config.useDualStack = true;
  config.endpointOverride = "localhost";
  return config;  
}

} // kclpp_test_support
