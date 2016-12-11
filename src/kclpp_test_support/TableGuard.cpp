#include "kclpp_test_support/TableGuard.h"
#include "kclpp_test_support/misc.h"
#include "kclpp/locks/ThreadBaton.h"
#include "kclpp/ScopeGuard.h"
#include "kclpp/dynamo/KCLDynamoClient.h"
#include <gtest/gtest.h>
#include <aws/dynamodb/model/DeleteTableRequest.h>

using kclpp::locks::ThreadBaton;
using kclpp::leases::LeaseManager;
using kclpp::dynamo::KCLDynamoClient;

using Aws::DynamoDB::Model::DeleteTableRequest;

namespace kclpp_test_support {

TableGuard::TableGuard(LeaseManagerTestContext *ctx): testContext_(ctx) {
  token_.mark();
}

TableGuard::TableGuard(TableGuard&& other)
  : testContext_(other.testContext_),
    token_(std::move(other.token_)) {
  other.testContext_ = nullptr;
}

TableGuard& TableGuard::operator=(TableGuard&& other) {
  std::swap(testContext_, other.testContext_);
  std::swap(token_, other.token_);
  return *this;
}

TableGuard TableGuard::create(LeaseManagerTestContext *ctx) {
  ThreadBaton bat1;
  ctx->manager->createTableIfNotExists(LeaseManager::CreateTableParams(),
    [&bat1](const LeaseManager::create_table_outcome_t& outcome) {
      auto guard = makePostGuard(bat1);
      EXPECT_TRUE(outcome.IsSuccess());
      EXPECT_TRUE(outcome.GetResult().created);
    }
  );
  ctx->loopOnBaton(bat1);
  return TableGuard {ctx};
}

TableGuard::~TableGuard() {
  if (token_) {
    // yes, this could throw in the destructor.
    // it's a test.
    // deal with it.
    token_.clear();
    ThreadBaton bat;
    testContext_->client->deleteTableAsync(
      DeleteTableRequest().WithTableName(testContext_->tableName),
      [&bat](const KCLDynamoClient::delete_table_outcome_t& outcome) {
        auto guard = makePostGuard(bat);
        EXPECT_TRUE(outcome.IsSuccess());
      }
    );
    testContext_->loopOnBaton(bat);
  }
}

} // kclpp_test_support