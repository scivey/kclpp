#include "kclpp_test_support/LeaseCoordinatorTestContext.h"
#include "kclpp_test_support/testConfig.h"
#include "kclpp_test_support/MockClock.h"

#include "kclpp/leases/LeaseManager.h"
#include "kclpp/dynamo/types.h"
#include <gtest/gtest.h>

using kclpp::leases::LeaseManager;
using kclpp::dynamo::TableName;
using kclpp::dynamo::KCLDynamoClient;
using kclpp::async::KCLAsyncContext;

namespace util = kclpp::util;

namespace kclpp_test_support {
LeaseCoordinatorTestContext::LeaseCoordinatorTestContext(
    manager_context_ptr_t mgrCtx) {
  managerContext = mgrCtx;
  leaseDuration = NanosecondDuration {MockClock::secToNanosec(30)};
  clock = std::make_shared<MockClock>();
  LeaseRenewer::LeaseRenewerParams renewerParams;
  renewerParams.clock = clock;
  renewerParams.workerID = workerID;
  renewerParams.leaseManager = managerContext->manager;
  renewerParams.asyncContext = managerContext->asyncContext;
  renewerParams.leaseDuration = leaseDuration;
  renewer = LeaseRenewer::createShared(std::move(renewerParams));
  LeaseTaker::LeaseTakerParams takerParams;
  takerParams.workerID = workerID;
  takerParams.leaseDuration = leaseDuration;
  takerParams.clock = clock;
  takerParams.asyncContext = managerContext->asyncContext;
  takerParams.leaseManager = managerContext->manager;
  taker = util::createShared<LeaseTaker>(std::move(takerParams));
  LeaseCoordinator::LeaseCoordinatorParams coordinatorParams;
  coordinatorParams.options.epsilon = std::chrono::milliseconds {50};
  coordinatorParams.options.leaseDurationMsec = std::chrono::milliseconds {30000};
  coordinatorParams.manager = managerContext->manager;
  coordinatorParams.taker = taker;
  coordinatorParams.renewer = renewer;
  coordinatorParams.asyncContext = managerContext->asyncContext;
  coordinator = util::createShared<LeaseCoordinator>(std::move(coordinatorParams));
}

LeaseCoordinatorTestContext::LeaseCoordinatorTestContext()
  : LeaseCoordinatorTestContext(std::make_shared<LeaseManagerTestContext>()) {}

LeaseCoordinatorTestContext::~LeaseCoordinatorTestContext() {
  coordinator.reset();
  renewer.reset();
  taker.reset();
  managerContext.reset();
  clock.reset();
}

std::shared_ptr<KCLAsyncContext> LeaseCoordinatorTestContext::getAsyncContext() {
  return managerContext->asyncContext;
}

} // kclpp_test_support
