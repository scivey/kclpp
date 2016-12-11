#pragma once
#include <memory>
#include <string>

#include "kclpp/locks/ThreadBaton.h"
#include "kclpp/leases/Lease.h"

namespace kclpp {
class ScopeGuard;

namespace leases {
  class LeaseManager;
  class Lease;
}

namespace async {
  class KCLAsyncContext;
}

}

namespace kclpp_test_support {

struct LeaseManagerTestContext;

kclpp::ScopeGuard makePostGuard(kclpp::locks::ThreadBaton&);

kclpp::leases::Lease::LeaseState buildSimpleLeaseState(
  const std::string& leaseKey,
  const std::string& leaseOwner,
  size_t leaseCounter
);

std::shared_ptr<kclpp::leases::Lease> buildSimpleLease(
  const std::string& leaseKey,
  const std::string& leaseOwner,
  size_t leaseCounter
);

void loopOnBaton(kclpp::async::KCLAsyncContext*, kclpp::locks::ThreadBaton& baton);



} // kclpp_test_support
