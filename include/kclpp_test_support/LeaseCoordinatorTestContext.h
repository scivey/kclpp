#pragma once

#include <memory>
#include <string>
#include "kclpp/dynamo/KCLDynamoClient.h"
#include "kclpp/leases/LeaseManager.h"
#include "kclpp/leases/LeaseCoordinator.h"
#include "kclpp/leases/LeaseTaker.h"
#include "kclpp/leases/types.h"
#include "kclpp/locks/ThreadBaton.h"
#include "kclpp/async/KCLAsyncContext.h"
#include "kclpp_test_support/LeaseManagerTestContext.h"

namespace kclpp_test_support {

struct LeaseCoordinatorTestContext {
  using Clock = kclpp::clock::Clock;
  using LeaseRenewer = kclpp::leases::LeaseRenewer;
  using LeaseTaker = kclpp::leases::LeaseTaker;
  using LeaseCoordinator = kclpp::leases::LeaseCoordinator;
  using LeaseOwner = kclpp::leases::LeaseOwner;
  using NanosecondDuration = kclpp::clock::NanosecondDuration;
  using KCLAsyncContext = kclpp::async::KCLAsyncContext;
  using manager_context_ptr_t = std::shared_ptr<LeaseManagerTestContext>;
  manager_context_ptr_t managerContext {nullptr};
  std::shared_ptr<Clock> clock {nullptr};
  std::shared_ptr<LeaseRenewer> renewer {nullptr};
  std::shared_ptr<LeaseTaker> taker {nullptr};
  std::shared_ptr<LeaseCoordinator> coordinator {nullptr};
  LeaseOwner workerID {"worker-1-id"};
  NanosecondDuration leaseDuration;
  LeaseCoordinatorTestContext();
  LeaseCoordinatorTestContext(manager_context_ptr_t);
  ~LeaseCoordinatorTestContext();
  std::shared_ptr<KCLAsyncContext> getAsyncContext();
};

} // kclpp_test_support
