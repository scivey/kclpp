#pragma once
#include <memory>
#include <atomic>
#include <functional>
#include <vector>

#include "kclpp/leases/Lease.h"
#include "kclpp/leases/types.h"
#include "kclpp/leases/LeaseManager.h"
#include "kclpp/leases/LeaseRenewer.h"
#include "kclpp/leases/LeaseTaker.h"
#include "kclpp/leases/LeaseCoordinatorIf.h"

#include "kclpp/clock/Clock.h"
#include "kclpp/leases/LeaseManager.h"
#include "kclpp/locks/Synchronized.h"
#include "kclpp/AsyncGroup.h"
#include "kclpp/async/KCLAsyncContext.h"
#include "kclpp/util/cas.h"

namespace kclpp { namespace leases {

class LeaseCoordinator: public LeaseCoordinatorIf {
 public:
  struct LeaseCoordinatorOptions {
    std::chrono::milliseconds epsilon {0};
    std::chrono::milliseconds leaseDurationMsec {0};
    LeaseManager::CreateTableParams defaultTableParams;
    LeaseManager::WaitUntilTableExistsParams tableExistsParams;
    std::chrono::milliseconds getRenewerInterval() const {
      auto base = leaseDurationMsec.count() / 3;
      DCHECK(base >= epsilon.count());
      return std::chrono::milliseconds {
        base - epsilon.count()
      };
    }
    std::chrono::milliseconds getTakerInterval() const {
      auto base = leaseDurationMsec.count() + epsilon.count();
      return std::chrono::milliseconds{ base * 2 };
    }
  };

  struct LeaseCoordinatorParams {
    std::shared_ptr<LeaseManager> manager {nullptr};
    std::shared_ptr<LeaseRenewer> renewer {nullptr};
    std::shared_ptr<LeaseTaker> taker {nullptr};
    std::shared_ptr<KCLAsyncContext> asyncContext {nullptr};
    LeaseCoordinatorOptions options;
  };

  struct LeaseCoordinatorState {
    LeaseCoordinatorParams params;
    std::unique_ptr<async::TimerEvent> takerTimer {nullptr};
    std::unique_ptr<async::TimerEvent> renewerTimer {nullptr};
    std::atomic<bool> running {false};

    bool setRunning(bool isCurrentlyRunning) {
      return util::casLoop(running, !isCurrentlyRunning, isCurrentlyRunning);
    }
  };

  using state_ptr_t = std::unique_ptr<LeaseCoordinatorState>;
 protected:
  state_ptr_t state_ {nullptr};
  LeaseCoordinator(state_ptr_t&& state);

  void runRenewer();
  void runTaker();
 public:
  static LeaseCoordinator* createNew(LeaseCoordinatorParams&& params);

  void start(unit_cb_t callback) override;
  LeaseManager* getManager() override;
  void stop(unit_cb_t callback) override;
  bool isRunning() const override;
  void updateLease(lease_ptr_t, const ConcurrencyToken&, update_lease_cb_t) override;

  Optional<lease_ptr_t> getCurrentlyHeldLease(LeaseKey);
  lease_manager_ptr_t getLeaseManager() override;
};

}} // kclpp::leases