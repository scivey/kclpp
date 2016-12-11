#include "kclpp_test_support/misc.h"
#include "kclpp_test_support/LeaseManagerTestContext.h"

#include <atomic>
#include <thread>
#include <chrono>
#include <gtest/gtest.h>
#include "kclpp/leases/types.h"
#include "kclpp/leases/LeaseManager.h"

#include "kclpp/ScopeGuard.h"


using namespace std;
using kclpp::ScopeGuard;
using kclpp::locks::ThreadBaton;
using kclpp::leases::LeaseManager;
using kclpp::leases::LeaseCounter;
using kclpp::leases::LeaseOwner;
using kclpp::leases::LeaseKey;
using kclpp::async::KCLAsyncContext;
using kclpp::leases::Lease;

namespace kclpp_test_support {

ScopeGuard makePostGuard(ThreadBaton& baton) {
  return kclpp::makeGuard([&baton]() {
    baton.post();
  });
}


Lease::LeaseState buildSimpleLeaseState(const string& leaseKey,
    const string& leaseOwner, size_t leaseCounter) {
  Lease::LeaseState leaseState;
  leaseState.leaseKey = LeaseKey {leaseKey};
  leaseState.leaseOwner.assign(LeaseOwner{leaseOwner});
  leaseState.leaseCounter.assign(LeaseCounter{leaseCounter});
  return leaseState;
}

std::shared_ptr<Lease> buildSimpleLease(const string& leaseKey,
    const string& leaseOwner, size_t leaseCounter) {
  return std::make_shared<Lease>(
    buildSimpleLeaseState(leaseKey, leaseOwner, leaseCounter)
  );
}

void loopOnBaton(KCLAsyncContext* asyncContext, ThreadBaton& baton) {
  while (!baton.isDone()) {
    asyncContext->getEventContext()->getBase()->runOnce();
  }
}


} // kclpp_test_support
