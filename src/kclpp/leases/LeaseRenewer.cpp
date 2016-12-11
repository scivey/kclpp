#include "kclpp/leases/LeaseRenewer.h"
#include <memory>
#include <atomic>
#include <functional>
#include <vector>

#include "kclpp/dynamo/KCLDynamoClient.h"
#include "kclpp/leases/Lease.h"
#include "kclpp/leases/types.h"
#include "kclpp/clock/Clock.h"
#include "kclpp/leases/LeaseManager.h"
#include "kclpp/locks/Synchronized.h"
#include "kclpp/AsyncGroup.h"

namespace kclpp { namespace leases {

using lease_ptr_t = LeaseRenewer::lease_ptr_t;
using LeaseRenewerState = LeaseRenewer::LeaseRenewerState;
using lease_map_t = LeaseRenewer::lease_map_t;

LeaseRenewer::LeaseRenewer(LeaseRenewerState&& initialState)
  : state_(std::forward<LeaseRenewerState>(initialState)){}


LeaseRenewerState& LeaseRenewer::dangerouslyGetState() {
  return state_;
}

std::shared_ptr<LeaseRenewer> LeaseRenewer::createShared(LeaseRenewerParams&& params) {
  LeaseRenewerState state;
  CHECK(!!params.leaseManager);
  CHECK(!!params.clock);
  CHECK(!!params.asyncContext);
  state.params = std::forward<LeaseRenewerParams>(params);
  return std::shared_ptr<LeaseRenewer>{new LeaseRenewer{std::move(state)}};
}

void LeaseRenewer::addLeasesToRenew(const std::vector<lease_ptr_t> leases) {
  if (!leases.empty()) {
    for (auto& leasePtr: leases) {
      if (!leasePtr->getState().lastCounterIncrementNanoseconds.hasValue()) {
        continue;
      }
      auto copied = leasePtr->copySharedWithNewConcurrencyToken();
      state_.ownedLeases.insert(std::make_pair(
        copied->getState().leaseKey, copied
      ));
    }
  }
}

void LeaseRenewer::clearHeldLeases() {
  state_.ownedLeases.clear();
}

void LeaseRenewer::dropLease(const lease_ptr_t& lease) {
  auto leaseKey = lease->getState().leaseKey;
  if (state_.ownedLeases.count(leaseKey) > 0) {
    state_.ownedLeases.erase(leaseKey);
  }
}


void LeaseRenewer::replaceRenewedLease(lease_ptr_t lease) {
  auto leaseKey = lease->getState().leaseKey;
  auto& leases = state_.ownedLeases;
  auto found = leases.find(leaseKey);
  if (found == leases.end()) {
    leases.insert(std::make_pair(leaseKey, lease));
  } else {
    found->second = lease;
  }
}

void LeaseRenewer::safeInsertLease(const lease_ptr_t& lease) {
  auto leaseKey = lease->getState().leaseKey;
  auto& leases = state_.ownedLeases;
  if (leases.count(leaseKey) == 0) {
    leases.insert(std::make_pair(leaseKey, lease));
  } else {
    leases.at(leaseKey) = lease;
  }
}


void LeaseRenewer::renewLease(lease_ptr_t lease, const RenewLeaseOptions& options, renew_lease_cb_t callback) {
  // add a retry with backoff here
  state_.params.asyncContext->runInEventThread(
    [this, lease, options, callback = std::move(callback)]() {
      auto rightNow = state_.params.clock->getNow();
      bool shouldRenew = options.renewEvenIfExpired
          || (!lease->isExpired(state_.params.leaseDuration, rightNow));
      if (shouldRenew) {
        state_.params.leaseManager->renewLease(lease,
          [lease, callback, this]
          (const renew_lease_outcome_t& outcome) {
            if (outcome.IsSuccess()) {
              this->replaceRenewedLease(outcome.GetResult().lease);
            } else {
              this->dropLease(lease);
            }
            callback(outcome);
          }
        );
      } else {
        dropLease(lease);
        RenewLeaseResult result;
        result.renewed = false;
        result.lease = lease;
        callback(renew_lease_outcome_t{result});
      }
    }
  );
}

  
void LeaseRenewer::renewLeases(renew_leases_cb_t callback) {
  state_.params.asyncContext->runInEventThread(
    [this, callback = std::move(callback)]() {
      RenewLeaseOptions options;
      options.renewEvenIfExpired = false;
      std::shared_ptr<AsyncGroup> asyncContext {nullptr};
      std::vector<lease_ptr_t> toRenew;
      {
        auto& leases = state_.ownedLeases;
        const auto numLeases = leases.size();
        asyncContext.reset(new AsyncGroup{numLeases});
        toRenew.reserve(numLeases);
        for (auto& item: leases) {
          toRenew.emplace_back(item.second);
        }
      }
      DCHECK(!!asyncContext);
      for (auto& lease: toRenew) {
        renewLease(lease, options,
          [this, callback = std::move(callback), asyncContext]
          (const renew_lease_outcome_t& outcome) {
            if (!outcome.IsSuccess()) {
              // just logging this for now - not sure on what/any error conditions
              // are plausible here, since individual lease renewals
              // are allowed to fail.
              LOG(INFO) << "renewLeases error: '" << outcome.GetError().GetMessage() << "'";
            }
            asyncContext->post();
            if (asyncContext->isDone() && asyncContext->becomeLeader()) {
              RenewLeasesResult result;
              result.succeeded = true;
              callback(renew_leases_outcome_t{result});
            }
          }
        );
      }
    }
  );
}

lease_map_t LeaseRenewer::copyCurrentlyHeldLeases() {
  lease_map_t result;
  auto now = state_.params.clock->getNow();
  for (const auto& keyVal: state_.ownedLeases) {
    auto copiedLease = getCopyOfHeldLease(keyVal.first, now);
    if (copiedLease.hasValue()) {
      result.insert(std::make_pair(
        keyVal.first, copiedLease.value()
      ));
    }
  }
  return result;
}

Optional<lease_ptr_t> LeaseRenewer::getCurrentlyHeldLease(LeaseKey key) {
  auto now = state_.params.clock->getNow();
  return getCopyOfHeldLease(key, now);
}

Optional<lease_ptr_t> LeaseRenewer::getCopyOfHeldLease(LeaseKey key,
    NanosecondPoint nowTime) {
  Optional<lease_ptr_t> result;
  auto found = state_.ownedLeases.find(key);
  if (found == state_.ownedLeases.end()) {
    return result;
  }
  lease_ptr_t foundLease = found->second;
  if (foundLease->isExpired(state_.params.leaseDuration, nowTime)) {
    return result;
  }
  result.assign(foundLease->copyShared());
  return result;
}

void LeaseRenewer::initialize(initialize_cb_t callback) {
  state_.params.asyncContext->runInEventThread([this, callback]() {
    state_.params.leaseManager->listLeases(
      [this, callback = std::move(callback)]
      (const LeaseManager::list_leases_outcome_t& outcome) {
        if (!outcome.IsSuccess()) {
          callback(initialize_outcome_t{ outcome.GetError() });
          return;
        }
        auto leases = outcome.GetResult();
        if (leases.empty()) {
          InitializeResult result;
          result.success = true;
          callback(initialize_outcome_t{result});
          return;
        }
        auto asyncContext = std::make_shared<AsyncGroup>(leases.size());
        RenewLeaseOptions options;
        options.renewEvenIfExpired = true;
        for (const auto& lease: leases) {
          renewLease(lease, options,
            [this, callback, asyncContext]
            (const renew_lease_outcome_t& subOutcome) {
              if (subOutcome.IsSuccess()) {
                auto subResult = subOutcome.GetResult();
                if (subResult.renewed) {
                  this->safeInsertLease(subResult.lease);
                }
              }
              asyncContext->post();
              if (asyncContext->isDone()) {
                if (asyncContext->becomeLeader()) {
                  std::vector<lease_ptr_t> currentValues;
                  {
                    // handle needs its own scope here to avoid deadlock.
                    for (const auto& item: state_.ownedLeases) {
                      currentValues.push_back(item.second);
                    }
                  }
                  if (currentValues.size()) {
                    addLeasesToRenew(currentValues);
                  }
                  InitializeResult result;
                  result.success = true;
                  callback(initialize_outcome_t {result});
                }
              }
            }
          );
        }
      }
    );
  });
}

void LeaseRenewer::updateLease(lease_ptr_t lease, const ConcurrencyToken& token,
    update_lease_cb_t callback) {
  DCHECK(lease->getState().concurrencyToken.hasValue());
  DCHECK(lease->getState().concurrencyToken.value() == token);

  auto returnFalse = [callback, lease]() {
    LeaseManagerIf::UpdateLeaseResult result;
    result.updated = false;
    result.lease = lease; 
    callback(update_lease_outcome_t{std::move(result)});
  };

  auto authoritativeIter = state_.ownedLeases.find(lease->getState().leaseKey);
  if (authoritativeIter == state_.ownedLeases.end()) {
    // not owned
    LOG(INFO) << "attempting to update non-owned lease; aborting.";
    returnFalse();
    return;
  }
  lease_ptr_t& authoritative = authoritativeIter->second;
  if (authoritative->getState().concurrencyToken.hasValue()) {
    if (authoritative->getState().concurrencyToken.value() != token) {
      LOG(INFO) << "non-matching concurrencyToken on lease update; aborting.";
      returnFalse();
      return;
    }
  }

  state_.params.leaseManager->updateLease(lease,
    [this, callback](const auto& outcome) {
      if (!outcome.IsSuccess()) {
        callback(outcome);
        return;
      }
      const auto& result = outcome.GetResult();
      if (result.updated) {
        safeInsertLease(result.lease);
      }
      callback(result);
    }
  );
}

}} // kclpp::leases