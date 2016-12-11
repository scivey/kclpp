#include "kclpp/leases/LeaseCoordinator.h"
#include <memory>
#include <atomic>
#include <functional>
#include <vector>

#include "kclpp/leases/Lease.h"
#include "kclpp/leases/types.h"
#include "kclpp/leases/LeaseManager.h"
#include "kclpp/leases/LeaseRenewer.h"
#include "kclpp/leases/LeaseTaker.h"
#include "kclpp/clock/Clock.h"
#include "kclpp/leases/LeaseManager.h"
#include "kclpp/locks/Synchronized.h"
#include "kclpp/AsyncGroup.h"
#include "kclpp/async/KCLAsyncContext.h"

namespace kclpp { namespace leases {
using async::TimerEvent;

using state_ptr_t = LeaseCoordinator::state_ptr_t;
using LeaseCoordinatorParams = LeaseCoordinator::LeaseCoordinatorParams;
using LeaseCoordinatorState = LeaseCoordinator::LeaseCoordinatorState;
using LeaseCoordinatorOptions = LeaseCoordinator::LeaseCoordinatorOptions;
using unit_outcome_t = LeaseCoordinator::unit_outcome_t;
using unit_cb_t = LeaseCoordinator::unit_cb_t;
using lease_manager_ptr_t = LeaseCoordinator::lease_manager_ptr_t;
using lease_ptr_t = LeaseCoordinator::lease_ptr_t;

LeaseCoordinator::LeaseCoordinator(state_ptr_t&& state): state_(std::move(state)) {}

LeaseCoordinator* LeaseCoordinator::createNew(LeaseCoordinatorParams&& params) {
  state_ptr_t coordinatorState = std::make_unique<LeaseCoordinatorState>();
  coordinatorState->params = std::move(params);
  return new LeaseCoordinator(std::move(coordinatorState));
}


void LeaseCoordinator::start(unit_cb_t callback) {
  DCHECK(!!state_);
  CHECK(state_->setRunning(true));
  state_->params.renewer->initialize(
    [this, callback = std::move(callback)](const auto& outcome) {
      LOG(INFO) << "initialized";
      if (!outcome.IsSuccess()) {
        callback(unit_outcome_t{outcome.GetError()});
        return;
      }
      DCHECK(outcome.GetResult().success);
      bool persistTimer {true};
      auto takerTimer = util::createUnique<TimerEvent>(
        state_->params.asyncContext->getEventContext()->getBase(),
        persistTimer
      );
      takerTimer->setHandler([this](){ runTaker(); });
      takerTimer->add(TimerSettings{
        state_->params.options.getTakerInterval()
      });
      state_->takerTimer = std::move(takerTimer);

      auto renewerTimer = util::createUnique<TimerEvent>(
        state_->params.asyncContext->getEventContext()->getBase(),
        persistTimer
      );
      renewerTimer->setHandler([this](){ runRenewer(); });
      renewerTimer->add(TimerSettings{
        state_->params.options.getRenewerInterval()
      });
      state_->renewerTimer = std::move(renewerTimer);
      callback(unit_outcome_t{Unit{}});
    }
  );

}

LeaseManager* LeaseCoordinator::getManager() {
  DCHECK(!!state_);
  return state_->params.manager.get();
}

void LeaseCoordinator::stop(unit_cb_t callback) {
  CHECK(state_->setRunning(false));
  state_->params.asyncContext->runInEventThread(
    [this, callback = std::move(callback)]() {
      DCHECK(!!state_->renewerTimer);
      DCHECK(!!state_->takerTimer);
      state_->renewerTimer->del();
      state_->takerTimer->del();
      callback(unit_outcome_t{Unit{}});
    }
  );
}

bool LeaseCoordinator::isRunning() const {
  return state_->running.load();
}

void LeaseCoordinator::runTaker() {
  state_->params.taker->takeLeases([this](const auto& takeOutcome) {
    CHECK(takeOutcome.IsSuccess());
    auto result = takeOutcome.GetResult();
    if (this->isRunning()) {
      state_->params.renewer->addLeasesToRenew(result.leases);
    }
  });
}

void LeaseCoordinator::runRenewer() {
  state_->params.renewer->renewLeases([this](const auto& renewOutcome) {
    CHECK(renewOutcome.IsSuccess());
    auto result = renewOutcome.GetResult();
    CHECK(result.succeeded);
  });
}

void LeaseCoordinator::updateLease(lease_ptr_t lease,
    const ConcurrencyToken& token, update_lease_cb_t callback) {
  state_->params.renewer->updateLease(lease, token, callback);
}

Optional<lease_ptr_t> LeaseCoordinator::getCurrentlyHeldLease(LeaseKey key) {
  return state_->params.renewer->getCurrentlyHeldLease(key);
}

lease_manager_ptr_t LeaseCoordinator::getLeaseManager() {
  return state_->params.manager;
}

}} // kclpp::leases