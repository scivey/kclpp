#include <gtest/gtest.h>
#include <aws/dynamodb/DynamoDBClient.h>
#include <aws/dynamodb/model/ListTablesRequest.h>
#include <aws/dynamodb/model/ListTablesResult.h>
#include "kclpp/leases/LeaseManager.h"
#include "kclpp/leases/LeaseRenewer.h"
#include "kclpp/leases/Lease.h"
#include "kclpp/leases/LeaseTaker.h"
#include "kclpp/leases/types.h"

#include "kclpp/dynamo/KCLDynamoClient.h"
#include "kclpp/APIGuard.h"
#include "kclpp/ScopeGuard.h"
#include "kclpp/UniqueToken.h"
#include "kclpp/clock/SystemClock.h"
#include "kclpp/locks/ThreadBaton.h"
#include "kclpp/util/misc.h"

#include "kclpp_test_support/TestRetryStrategy.h"
#include "kclpp_test_support/LeaseManagerTestContext.h"
#include "kclpp_test_support/misc.h"
#include "kclpp_test_support/TableGuard.h"
#include "kclpp_test_support/MockClock.h"
#include "kclpp_test_support/MockClock.h"
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


struct BaseTakerTestContext {
  std::shared_ptr<LeaseManagerTestContext> managerContext {nullptr};
  MockClock mockClock;
  std::shared_ptr<Clock> clock {nullptr};
  LeaseOwner workerID {"worker-1-id"};
  NanosecondDuration leaseDuration {MockClock::secToNanosec(30)};
  std::shared_ptr<LeaseTaker> taker {nullptr};
  BaseTakerTestContext() {
    managerContext = std::make_shared<LeaseManagerTestContext>();
    clock = std::shared_ptr<Clock> {
      &mockClock, [](Clock *target) {
        // do nothing
      }
    };
    LeaseTaker::LeaseTakerParams takerParams;
    takerParams.workerID = workerID;
    takerParams.leaseDuration = leaseDuration;
    takerParams.clock = clock;
    takerParams.maxLeasesForWorker = NumberOfLeases {10};
    takerParams.asyncContext = managerContext->asyncContext;
    takerParams.maxLeasesToStealAtOnce = NumberOfLeases {1};
    takerParams.leaseManager = managerContext->manager; 
    taker = util::createShared<LeaseTaker>(std::move(takerParams));
  }
  ~BaseTakerTestContext() {
    taker.reset();
    managerContext.reset();
    clock.reset();
  }
};


auto makeRenewer(BaseTakerTestContext* ctx) {
  LeaseRenewer::LeaseRenewerParams renewerParams;
  renewerParams.clock = ctx->clock;
  renewerParams.workerID = ctx->workerID;
  renewerParams.leaseManager = ctx->managerContext->manager;;
  renewerParams.leaseDuration = ctx->leaseDuration;
  renewerParams.asyncContext = ctx->managerContext->asyncContext;
  return LeaseRenewer::createShared(std::move(renewerParams));  
}

TEST(TestLeaseTaker, TestInitialize1) {
  BaseTakerTestContext testContext;
  {
    auto tableGuard = TableGuard::create(testContext.managerContext.get());
    {
      auto setTimeAndSave = [&testContext](Lease::LeaseState&& leaseState) {
        leaseState.lastCounterIncrementNanoseconds.assign(NanosecondPoint{ MockClock::secToNanosec(1000) });
        auto lease = std::make_shared<Lease>(std::move(leaseState));
        testContext.managerContext->saveLeaseSync(lease);
      };
      setTimeAndSave(buildSimpleLeaseState("shard-1", "worker-1-id", 10));
      setTimeAndSave(buildSimpleLeaseState("shard-2", "worker-1-id", 15));
      setTimeAndSave(buildSimpleLeaseState("shard-3", "worker-1-id", 6));
      setTimeAndSave(buildSimpleLeaseState("shard-4", "worker-1-id", 21));
      testContext.managerContext->pollUntilAnyLeaseExists();
    }
    testContext.mockClock.setNow(NanosecondPoint{MockClock::secToNanosec(1002)});
    {
      auto renewer = makeRenewer(&testContext);
      auto taker = testContext.taker;
      {
        ThreadBaton bat1;
        renewer->initialize([&bat1](const auto& outcome) {
          auto guard = makePostGuard(bat1);
          EXPECT_TRUE(outcome.IsSuccess());
          auto result = outcome.GetResult();
          EXPECT_TRUE(result.success);
        });
        testContext.managerContext->loopOnBaton(bat1);
      }
      {
        ThreadBaton bat;
        taker->takeLeases([&bat](const auto& outcome) {
          auto guard = makePostGuard(bat);
          EXPECT_TRUE(outcome.IsSuccess());
        });
        testContext.managerContext->loopOnBaton(bat);
      }
    }
  }
}

