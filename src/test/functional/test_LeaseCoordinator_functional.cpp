#include <gtest/gtest.h>
#include <aws/dynamodb/DynamoDBClient.h>
#include <aws/dynamodb/model/ListTablesRequest.h>
#include <aws/dynamodb/model/ListTablesResult.h>
#include "kclpp/leases/LeaseManager.h"
#include "kclpp/leases/LeaseRenewer.h"
#include "kclpp/leases/LeaseCoordinator.h"
#include "kclpp/leases/Lease.h"
#include "kclpp/dynamo/KCLDynamoClient.h"
#include "kclpp/APIGuard.h"
#include "kclpp/ScopeGuard.h"
#include "kclpp/UniqueToken.h"
#include "kclpp/clock/SystemClock.h"
#include "kclpp/locks/ThreadBaton.h"
#include "kclpp_test_support/TestRetryStrategy.h"
#include "kclpp_test_support/LeaseManagerTestContext.h"
#include "kclpp_test_support/LeaseCoordinatorTestContext.h"
#include "kclpp_test_support/misc.h"
#include "kclpp_test_support/MockClock.h"
#include "kclpp_test_support/TableGuard.h"

using namespace std;
using namespace kclpp::dynamo;
using namespace kclpp;
using namespace kclpp::leases;
using kclpp::locks::ThreadBaton;
using kclpp::clock::NanosecondPoint;
using kclpp::clock::NanosecondDuration;
using kclpp::clock::Clock;
using kclpp::clock::SystemClock;

using Aws::Client::ClientConfiguration;
using Aws::DynamoDB::Model::ListTablesRequest;
using Aws::DynamoDB::Model::GetItemRequest;
using Aws::DynamoDB::Model::DeleteTableRequest;
using kclpp_test_support::LeaseManagerTestContext;
using kclpp_test_support::LeaseCoordinatorTestContext;
using kclpp_test_support::makePostGuard;
using kclpp_test_support::TableGuard;
using kclpp_test_support::buildSimpleLease;
using kclpp_test_support::buildSimpleLeaseState;
using kclpp_test_support::MockClock;



TEST(TestLeaseCoordinator, TestStartStop) {
  LeaseCoordinatorTestContext context;
  {
    auto tableGuard = TableGuard::create(context.managerContext.get());
    {
      ThreadBaton bat;
      context.coordinator->start([&bat](const auto& outcome) {
        auto guard = makePostGuard(bat);
        EXPECT_TRUE(outcome.IsSuccess())
          << outcome.GetError().GetMessage();      
      });
      LOG(INFO) << "start 1.";
      context.managerContext->loopOnBaton(bat);
      LOG(INFO) << "start 2.";
    }
    {
      ThreadBaton bat;
      context.coordinator->stop([&bat](const auto& outcome) {
        auto guard = makePostGuard(bat);
        EXPECT_TRUE(outcome.IsSuccess())
          << outcome.GetError().GetMessage();
      });
      LOG(INFO) << "stop 1.";
      context.managerContext->loopOnBaton(bat);
      LOG(INFO) << "stop 2.";

    }
  }
}
