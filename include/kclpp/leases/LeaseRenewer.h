#pragma once
#include <memory>
#include <atomic>
#include <functional>
#include <vector>

#include "kclpp/dynamo/KCLDynamoClient.h"
#include "kclpp/leases/Lease.h"
#include "kclpp/leases/types.h"
#include "kclpp/leases/LeaseRenewerIf.h"

#include "kclpp/clock/Clock.h"
#include "kclpp/leases/LeaseManager.h"
#include "kclpp/locks/Synchronized.h"
#include "kclpp/AsyncGroup.h"

namespace kclpp { namespace async {
class KCLAsyncContext;
}} // kclpp::async

namespace kclpp { namespace leases {


class LeaseRenewer: public LeaseRenewerIf {
 public:
  struct LeaseRenewerParams {
    std::shared_ptr<async::KCLAsyncContext> asyncContext {nullptr};
    LeaseOwner workerID;
    NanosecondDuration leaseDuration;
    lease_manager_ptr_t leaseManager {nullptr};
    clock_ptr_t clock {nullptr};
  };

  struct LeaseRenewerState {
    LeaseRenewerParams params;
    lease_map_t ownedLeases;
  };

 protected:
  LeaseRenewerState state_;
  LeaseRenewer(LeaseRenewerState&& initialState);

 public:

  // exposed for testing
  LeaseRenewerState& dangerouslyGetState();

  static std::shared_ptr<LeaseRenewer> createShared(LeaseRenewerParams&& params);
  void addLeasesToRenew(const std::vector<lease_ptr_t> leases) override;
  void clearHeldLeases() override;
  void dropLease(const lease_ptr_t& lease) override;

 protected:

  void replaceRenewedLease(lease_ptr_t lease);
  void safeInsertLease(const lease_ptr_t& lease);

 public:
  void renewLease(lease_ptr_t, const RenewLeaseOptions&, renew_lease_cb_t) override;  
  void renewLeases(renew_leases_cb_t callback) override;
  lease_map_t copyCurrentlyHeldLeases() override;
  Optional<lease_ptr_t> getCurrentlyHeldLease(LeaseKey key) override;
  Optional<lease_ptr_t> getCopyOfHeldLease(LeaseKey key, NanosecondPoint nowTime) override;
  void initialize(initialize_cb_t callback) override;
  void updateLease(lease_ptr_t, const ConcurrencyToken&, update_lease_cb_t) override;
};

}} // kclpp::leases