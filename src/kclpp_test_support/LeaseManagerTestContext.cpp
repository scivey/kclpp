#include "kclpp_test_support/LeaseManagerTestContext.h"
#include "kclpp_test_support/testConfig.h"
#include "kclpp_test_support/misc.h"

#include "kclpp/leases/LeaseManager.h"
#include "kclpp/dynamo/types.h"
#include "kclpp/TimeLogger.h"
#include "kclpp/locks/ThreadBaton.h"

#include <gtest/gtest.h>

using namespace std;
using kclpp::leases::LeaseManager;
using kclpp::leases::Lease;
using kclpp::dynamo::TableName;
using kclpp::dynamo::KCLDynamoClient;
using kclpp::locks::ThreadBaton;

namespace util = kclpp::util;
using kclpp::TimeLogger;

namespace kclpp_test_support {

LeaseManagerTestContext::LeaseManagerTestContext(bool consistentReads) {
  asyncContext = util::createShared<kclpp::async::KCLAsyncContext>();
  KCLDynamoClient::KCLDynamoClientParams clientParams;
  clientParams.asyncContext = asyncContext;
  clientParams.awsConfig = makeClientConfig();
  client = util::createShared<KCLDynamoClient>(std::move(clientParams));
  LeaseManager::LeaseManagerParams managerParams;
  managerParams.tableName = TableName {tableName};
  managerParams.dynamoClient = client;
  managerParams.asyncContext = asyncContext;
  managerParams.consistentReads = consistentReads;
  auto managerOutcome = LeaseManager::createShared(std::move(managerParams));
  EXPECT_TRUE(managerOutcome.IsSuccess());
  manager = managerOutcome.GetResultWithOwnership();
  asyncContext->getEventContext()->getBase()->runOnce();
  asyncContext->getThreadPool()->start();
  asyncContext->getEventContext()->getBase()->runOnce();  
}

LeaseManagerTestContext::~LeaseManagerTestContext() {
  client.reset();
  manager.reset();
  asyncContext.reset();
}


void LeaseManagerTestContext::loopOnBaton(kclpp::locks::ThreadBaton& baton) {
  kclpp_test_support::loopOnBaton(
    asyncContext.get(), baton
  );
}

void LeaseManagerTestContext::pollUntilAnyLeaseExists() {
  std::atomic<bool> listSucceeded {false};
  std::atomic<size_t> loopCounter {0};
  while (loopCounter.load() < 10) {
    loopCounter++;
    ThreadBaton bat;
    manager->listLeases(
      [&bat, &listSucceeded](const auto& outcome) {
        auto guard = makePostGuard(bat);
        if (outcome.IsSuccess()) {
          auto result = outcome.GetResult();
          if (result.empty()) {
            return;
          }
          listSucceeded.store(true);
        }
      }
    );
    loopOnBaton(bat);
    if (listSucceeded.load()) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds {10});
  }
  EXPECT_TRUE(listSucceeded.load());
}

void LeaseManagerTestContext::saveLeaseSync(shared_ptr<Lease> lease) {
  ThreadBaton bat;
  manager->createLeaseIfNotExists(lease,
    [&bat](const auto& outcome) {
      auto guard = makePostGuard(bat);
      EXPECT_TRUE(outcome.IsSuccess())
        << "failure: '" << outcome.GetError().GetMessage() << "'";
      EXPECT_TRUE(outcome.GetResult().created);
    }
  );
  loopOnBaton(bat);
}

} // kclpp_test_support
