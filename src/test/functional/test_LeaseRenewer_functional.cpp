#include <gtest/gtest.h>
#include <aws/dynamodb/DynamoDBClient.h>
#include <aws/dynamodb/model/ListTablesRequest.h>
#include <aws/dynamodb/model/ListTablesResult.h>
#include "kclpp/leases/LeaseManager.h"
#include "kclpp/leases/LeaseRenewer.h"
#include "kclpp/leases/Lease.h"
#include "kclpp/dynamo/KCLDynamoClient.h"
#include "kclpp/APIGuard.h"
#include "kclpp/ScopeGuard.h"
#include "kclpp/UniqueToken.h"
#include "kclpp/clock/SystemClock.h"
#include "kclpp/locks/ThreadBaton.h"
#include "kclpp_test_support/TestRetryStrategy.h"
#include "kclpp_test_support/LeaseManagerTestContext.h"
#include "kclpp_test_support/misc.h"
#include "kclpp_test_support/TableGuard.h"
#include "kclpp_test_support/MockClock.h"

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
using kclpp_test_support::makePostGuard;
using kclpp_test_support::TableGuard;
using kclpp_test_support::MockClock;
using kclpp_test_support::buildSimpleLease;
using kclpp_test_support::buildSimpleLeaseState;


TEST(TestLeaseRenewer, TestInitialize1) {
  LeaseManagerTestContext context;
  auto leaseClock = std::make_shared<MockClock>();
  LeaseRenewer::LeaseRenewerParams renewerParams;
  renewerParams.clock = leaseClock;
  renewerParams.workerID = LeaseOwner {"worker-1-id"};
  renewerParams.asyncContext = context.asyncContext;
  renewerParams.leaseManager = context.manager;
  renewerParams.leaseDuration = NanosecondDuration { MockClock::secToNanosec(30) };
  auto renewer = LeaseRenewer::createShared(std::move(renewerParams));
  {
    auto tableGuard = TableGuard::create(&context);
    auto setTimeAndSave = [&context](Lease::LeaseState&& leaseState) {
      leaseState.lastCounterIncrementNanoseconds.assign(NanosecondPoint{ MockClock::secToNanosec(1000) });
      auto lease = std::make_shared<Lease>(std::move(leaseState));
      context.saveLeaseSync(lease);
    };
    setTimeAndSave(buildSimpleLeaseState("shard-1", "worker-1-id", 10));
    setTimeAndSave(buildSimpleLeaseState("shard-2", "worker-1-id", 15));
    setTimeAndSave(buildSimpleLeaseState("shard-3", "worker-1-id", 6));

    setTimeAndSave(buildSimpleLeaseState("shard-4", "worker-1-id", 21));
    context.pollUntilAnyLeaseExists();
    leaseClock->setNow(NanosecondPoint{MockClock::secToNanosec(1002)});
    {
      ThreadBaton baton;
      renewer->initialize([&baton](const LeaseRenewer::initialize_outcome_t& outcome) {
        auto guard = makePostGuard(baton);
        EXPECT_TRUE(outcome.IsSuccess());
        auto result = outcome.GetResult();
        EXPECT_TRUE(result.success);
      });
      context.loopOnBaton(baton);
    }
    {
      auto numLeases = renewer->dangerouslyGetState().ownedLeases.size();
      EXPECT_EQ(4, numLeases);
      auto copied = renewer->copyCurrentlyHeldLeases();
      EXPECT_EQ(4, copied.size());
    }
  }
}

TEST(TestLeaseRenewer, TestInitialize2) {
  LeaseManagerTestContext context;
  auto leaseClock = std::make_shared<MockClock>();
  LeaseRenewer::LeaseRenewerParams renewerParams;
  renewerParams.clock = leaseClock;
  renewerParams.workerID = LeaseOwner {"worker-1-id"};
  renewerParams.leaseManager = context.manager;
  renewerParams.asyncContext = context.asyncContext;
  renewerParams.leaseDuration = NanosecondDuration { MockClock::secToNanosec(30) };
  auto renewer = LeaseRenewer::createShared(std::move(renewerParams));
  {
    auto tableGuard = TableGuard::create(&context);
    auto saveLease = [&context] (const string& name, const string& worker,
        size_t counter, size_t secAge) {
      auto leaseState = buildSimpleLeaseState(name, worker, counter);
      leaseState.lastCounterIncrementNanoseconds.assign(NanosecondPoint{
        MockClock::secToNanosec(secAge)
      });
      context.saveLeaseSync(std::make_shared<Lease>(std::move(leaseState)));
    };

    saveLease("shard-1", "worker-1-id", 10, 985);
    saveLease("shard-2", "worker-1-id", 10, 997);
    saveLease("shard-3", "worker-1-id", 10, 800);
    saveLease("shard-4", "worker-1-id", 10, 991);
    context.pollUntilAnyLeaseExists();
    leaseClock->setNow(NanosecondPoint{MockClock::secToNanosec(1002)});
    {
      ThreadBaton baton;
      renewer->initialize([&baton](const LeaseRenewer::initialize_outcome_t& outcome) {
        auto guard = makePostGuard(baton);
        EXPECT_TRUE(outcome.IsSuccess());
        auto result = outcome.GetResult();
        EXPECT_TRUE(result.success);
      });
      context.loopOnBaton(baton);
    }
    {
      auto leases = renewer->copyCurrentlyHeldLeases();
      EXPECT_EQ(3, leases.size());
    }
  }
}

TEST(TestLeaseRenewer, TestRenewLeases1) {
  LeaseManagerTestContext context;
  auto leaseClock = std::make_shared<MockClock>();
  LeaseRenewer::LeaseRenewerParams renewerParams;
  renewerParams.clock = leaseClock;
  renewerParams.workerID = LeaseOwner {"worker-1-id"};
  renewerParams.leaseManager = context.manager;
  renewerParams.leaseDuration = NanosecondDuration { MockClock::secToNanosec(30) };
  renewerParams.asyncContext = context.asyncContext;
  auto renewer = LeaseRenewer::createShared(std::move(renewerParams));
  {
    auto tableGuard = TableGuard::create(&context);
    auto saveLease = [&context] (const string& name, const string& worker,
        size_t counter, size_t secAge) {
      auto leaseState = buildSimpleLeaseState(name, worker, counter);
      leaseState.lastCounterIncrementNanoseconds.assign(NanosecondPoint{
        MockClock::secToNanosec(secAge)
      });
      context.saveLeaseSync(std::make_shared<Lease>(std::move(leaseState)));
    };

    saveLease("shard-1", "worker-1-id", 10, 980);
    saveLease("shard-2", "worker-1-id", 20, 996);
    saveLease("shard-3", "worker-1-id", 30, 998);
    saveLease("shard-4", "worker-1-id", 40, 997);
    context.pollUntilAnyLeaseExists();
    leaseClock->setNow(NanosecondPoint{MockClock::secToNanosec(1000)});
    {
      ThreadBaton baton;
      renewer->initialize([&baton](const LeaseRenewer::initialize_outcome_t& outcome) {
        auto guard = makePostGuard(baton);
        EXPECT_TRUE(outcome.IsSuccess());
        auto result = outcome.GetResult();
        EXPECT_TRUE(result.success);
      });
      context.loopOnBaton(baton);
    }
    context.pollUntilAnyLeaseExists();
    {
      auto leases = renewer->copyCurrentlyHeldLeases();
      EXPECT_EQ(4, leases.size());
    }
    leaseClock->setNow(NanosecondPoint{MockClock::secToNanosec(1011)});
    {
      ThreadBaton baton;
      auto req = GetItemRequest()
        .WithTableName("table_1")
        .WithKey(dynamo_helpers::makeLeaseKey("shard-1"));
      context.client->getItemAsync(std::move(req),
        [&baton](const KCLDynamoClient::get_item_outcome_t& outcome) {
          auto guard = makePostGuard(baton);
          EXPECT_TRUE(outcome.IsSuccess());
        }
      );
      context.loopOnBaton(baton);
    }
    {
      ThreadBaton baton;
      renewer->renewLeases([&baton](const LeaseRenewer::renew_leases_outcome_t& outcome) {
        auto guard = makePostGuard(baton);
        EXPECT_TRUE(outcome.IsSuccess());
      });
      context.loopOnBaton(baton);
    }
  }
}

