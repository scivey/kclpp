#pragma once
#include <memory>
#include <atomic>
#include <functional>

#include "kclpp/dynamo/KCLDynamoClient.h"
#include "kclpp/leases/Lease.h"
#include "kclpp/leases/types.h"
#include "kclpp/clock/Clock.h"
#include "kclpp/leases/LeaseManager.h"
#include "kclpp/leases/LeaseTakerIf.h"
#include "kclpp/locks/Synchronized.h"
#include "kclpp/Unit.h"

namespace kclpp { namespace async {
class KCLAsyncContext;
}} // kclpp::async


namespace kclpp { namespace leases {

class LeaseTaker: public LeaseTakerIf {
 public:
  struct LeaseTakerParams {
    LeaseOwner workerID;
    NanosecondDuration leaseDuration {30000000000};
    lease_manager_ptr_t leaseManager {nullptr};
    clock_ptr_t clock {nullptr};
    NumberOfLeases maxLeasesForWorker {999};
    NumberOfLeases maxLeasesToStealAtOnce {1};
    std::shared_ptr<async::KCLAsyncContext> asyncContext {nullptr};
  };

  struct LeaseTakerState {
    LeaseTakerParams params;
    Optional<clock::NanosecondPoint> lastScanTime {NanosecondPoint{0}};
    lease_map_t allLeases;
    LeaseTakerState(){}
    LeaseTakerState(LeaseTakerParams&& params)
      : params(std::forward<LeaseTakerParams>(params)) {}
  };  
 protected:
  std::unique_ptr<LeaseTakerState> state_;

  LeaseTaker(std::unique_ptr<LeaseTakerState>&& initialState);
 public:
  static LeaseTaker* createNew(LeaseTakerParams&& params);

  void takeLeases(take_leases_cb_t) override;

  void updateAllLeases(update_all_leases_cb_t) override;

  std::vector<lease_ptr_t> getExpiredLeases() override;

  lease_count_map_t computeLeaseCounts(const std::set<LeaseKey>& expiredLeaseKeys) override;

  std::vector<lease_ptr_t> chooseLeasesToSteal(
    const lease_count_map_t&,
    const ChooseLeasesToStealOptions&
  ) override;
  
  std::vector<lease_ptr_t> computeLeasesToTake(const std::vector<lease_ptr_t>& expiredLeases) override;
};

}} // kclpp::leases