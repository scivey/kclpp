#pragma once

#include <memory>
#include <string>
#include "kclpp/dynamo/KCLDynamoClient.h"
#include "kclpp/leases/LeaseManager.h"
#include "kclpp/leases/Lease.h"

#include "kclpp/locks/ThreadBaton.h"

#include "kclpp/async/KCLAsyncContext.h"
#include <aws/core/client/RetryStrategy.h>
#include <aws/dynamodb/model/GetItemRequest.h>
#include <aws/dynamodb/model/GetItemResult.h>

namespace kclpp_test_support {

struct LeaseManagerTestContext {
  std::shared_ptr<kclpp::async::KCLAsyncContext> asyncContext {nullptr};
  std::shared_ptr<kclpp::dynamo::KCLDynamoClient> client {nullptr};
  std::shared_ptr<kclpp::leases::LeaseManager> manager {nullptr};
  const std::string tableName {"table_1"};
  LeaseManagerTestContext(bool consistentReads = false);
  ~LeaseManagerTestContext();
  void loopOnBaton(kclpp::locks::ThreadBaton&);
  void pollUntilAnyLeaseExists();
  void saveLeaseSync(std::shared_ptr<kclpp::leases::Lease> lease);
};

} // kclpp_test_support
