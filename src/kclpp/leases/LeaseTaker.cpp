#include "kclpp/leases/LeaseTaker.h"
#include <memory>
#include <atomic>
#include <functional>
#include <string>

#include "kclpp/dynamo/KCLDynamoClient.h"
#include "kclpp/leases/Lease.h"
#include "kclpp/leases/types.h"
#include "kclpp/clock/Clock.h"
#include "kclpp/leases/LeaseManager.h"
#include "kclpp/locks/Synchronized.h"
#include "kclpp/AsyncGroup.h"

using namespace std;

namespace kclpp { namespace leases {

using lease_ptr_t = LeaseTaker::lease_ptr_t;

LeaseTaker::LeaseTaker(unique_ptr<LeaseTakerState>&& initialState)
  : state_(std::move(initialState)) {}

LeaseTaker* LeaseTaker::createNew(LeaseTakerParams&& params) {
  CHECK(!!params.leaseManager);
  CHECK(!!params.clock);
  CHECK(!!params.asyncContext);
  return new LeaseTaker {
    std::unique_ptr<LeaseTakerState> {
      new LeaseTakerState(std::forward<LeaseTakerParams>(params))
    }
  };
}


struct TakeLeasesAsyncContext {
  AsyncGroup asyncGroup;
  vector<lease_ptr_t> takenLeases;
  TakeLeasesAsyncContext(size_t numWorkers): asyncGroup(numWorkers){}
};

void LeaseTaker::takeLeases(take_leases_cb_t callback) {
  updateAllLeases([callback, this](const update_all_leases_outcome_t& outcome) {
    if (!outcome.IsSuccess()) {
      callback(take_leases_outcome_t {outcome.GetError()} );
      return;
    }
    auto leasesToTake = this->computeLeasesToTake(this->getExpiredLeases());
    if (leasesToTake.empty()) {
      callback(take_leases_outcome_t{TakeLeasesResult{}});
      return;
    }
    auto asyncContext = std::make_shared<TakeLeasesAsyncContext>(leasesToTake.size());
    for (const auto& lease: leasesToTake) {
      this->state_->params.leaseManager->takeLease(lease, this->state_->params.workerID,
        [this, asyncContext, callback](const LeaseManager::take_lease_outcome_t& takeOutcome) {
          if (takeOutcome.IsSuccess() && takeOutcome.GetResult().taken) {
            auto lease = takeOutcome.GetResult().lease;
            DCHECK(!!lease);
            asyncContext->takenLeases.push_back(lease);
          }
          auto& asyncGroup = asyncContext->asyncGroup;
          asyncGroup.post();
          if (asyncGroup.isDone() && asyncGroup.becomeLeader()) {
            TakeLeasesResult result;
            result.leases = std::move(asyncContext->takenLeases);
            callback(take_leases_outcome_t {std::move(result)} );
          }
        }
      );
    }
  });
}

void LeaseTaker::updateAllLeases(update_all_leases_cb_t callback) {
  DCHECK(!!state_);
  DCHECK(!!state_->params.leaseManager);
  state_->params.leaseManager->listLeases(
    [this, callback = std::move(callback)]
    (const LeaseManager::list_leases_outcome_t& outcome) {
      if (!outcome.IsSuccess()) {
        callback(update_all_leases_outcome_t {outcome.GetError()} );
        return;
      }
      auto updatedLeases = outcome.GetResult();
      this->state_->lastScanTime.assign(this->state_->params.clock->getNow());
      {      
        auto notUpdated = util::keySet(state_->allLeases);
        for (auto& newLease: updatedLeases) {
          auto leaseKey = newLease->getState().leaseKey;
          util::removeIfExists(notUpdated, leaseKey);
          auto oldLease = util::findOption(state_->allLeases, leaseKey);
          Lease::LeaseState newLeaseState = newLease->getState();
          if (oldLease.hasValue()) {
            const auto& oldState = oldLease.value()->getState();
            if (oldState.leaseCounter == newLeaseState.leaseCounter) {
              newLeaseState.lastCounterIncrementNanoseconds = oldState.lastCounterIncrementNanoseconds;
            } else {
              newLeaseState.lastCounterIncrementNanoseconds.assign(
                NanosecondPoint{this->state_->lastScanTime->value()}
              );
            }
          } else {
            if (newLeaseState.leaseOwner.hasValue()) {
              newLeaseState.lastCounterIncrementNanoseconds.assign(
                NanosecondPoint{ this->state_->lastScanTime.value()}
              );
            } else {
              newLeaseState.lastCounterIncrementNanoseconds.clear();
            }
          }
          auto replacedLease = std::make_shared<Lease>(std::move(newLeaseState));
          util::insertOrUpdate(state_->allLeases, leaseKey, replacedLease);
        }
        for (const auto& key: notUpdated) {
          util::removeIfExists(state_->allLeases, key);
        }
      }
      callback(update_all_leases_outcome_t{Unit{}});
    }
  );
}

vector<shared_ptr<Lease>> LeaseTaker::getExpiredLeases() {
  vector<shared_ptr<Lease>> result;
  NanosecondPoint asOfTime {0};
  if (state_->lastScanTime.hasValue()) {
    asOfTime = state_->lastScanTime.value();
  }
  for (const auto& leasePair: state_->allLeases) {
    const auto& lease = leasePair.second;
    if (lease->isExpired(state_->params.leaseDuration, asOfTime)) {
      result.emplace_back(lease);
    }
  }
  return result;
}

using lease_count_map_t = LeaseTaker::lease_count_map_t;
lease_count_map_t LeaseTaker::computeLeaseCounts(const set<LeaseKey>& expiredLeaseKeys) {
  lease_count_map_t result;
  for (const auto& leasePair: state_->allLeases) {
    const auto& lease = leasePair.second;
    const auto& leaseState = lease->getState();
    const auto& leaseKey = leaseState.leaseKey;
    if (expiredLeaseKeys.count(leaseKey) == 0) {
      auto owner = leaseState.leaseOwner;
      if (!owner.hasValue()) {
        continue;
      }
      util::insertOrIncrement(result, owner.value(), 1);
    }
  }
  auto myCount = util::findOption(result, state_->params.workerID);
  if (!myCount.hasValue()) {
    result.insert(std::make_pair(
      state_->params.workerID, 0
    ));
  }
  return result;
}


vector<lease_ptr_t> LeaseTaker::chooseLeasesToSteal(const lease_count_map_t& leaseCounts,
    const ChooseLeasesToStealOptions& options) {
  vector<lease_ptr_t> leasesToSteal;
  Optional<std::pair<LeaseOwner, size_t>> mostLoadedWorker;
  for (const auto& countPair: leaseCounts) {
    if (!mostLoadedWorker.hasValue() || mostLoadedWorker.value().second < countPair.second) {
      mostLoadedWorker.assign(countPair);
    }
  }
  if (!mostLoadedWorker.hasValue()) {
    return leasesToSteal;
  }
  size_t numToSteal {0};
  size_t nTarget = options.targetNum.value();
  size_t nNeeded = options.neededNum.value();
  if ((mostLoadedWorker.value().second >= nTarget) && nNeeded > 0) {
    auto leasesOverTarget = mostLoadedWorker.value().second - nTarget;
    numToSteal = std::min<size_t>(nNeeded, leasesOverTarget);
    if (nNeeded > 1 && numToSteal == 0) {
      numToSteal = 1;
    }
    numToSteal = std::min(numToSteal, state_->params.maxLeasesToStealAtOnce.value());
  }
  if (numToSteal == 0) {
    return leasesToSteal;
  }
  auto mostLoadedKey = mostLoadedWorker.value().first;
  vector<lease_ptr_t> candidates;
  {
    for (const auto& leasePair: state_->allLeases) {
      const auto& lease = leasePair.second;
      auto leaseOwner = lease->getState().leaseOwner;
      if (leaseOwner.hasValue() && mostLoadedKey == leaseOwner.value()) {
        candidates.emplace_back(lease);
      }
    }
  }
  util::randomShuffle(candidates);
  size_t toIndex = std::min<size_t>(candidates.size(), numToSteal);
  {
    size_t idx {0};
    for (auto& item: candidates) {
      if (idx >= toIndex) {
        break;
      }
      leasesToSteal.emplace_back(std::move(item));
      idx++;
    }
  }
  return leasesToSteal;
}

vector<lease_ptr_t> LeaseTaker::computeLeasesToTake(const vector<lease_ptr_t>& expiredLeases) {
  set<LeaseKey> expiredKeys;
  for (const auto& lease: expiredLeases) {
    expiredKeys.insert(lease->getState().leaseKey);
  }
  auto leaseCounts = computeLeaseCounts(expiredKeys);
  vector<lease_ptr_t> leasesToTake;
  auto numLeases = state_->allLeases.size();
  auto numWorkers = leaseCounts.size();
  if (numLeases == 0) {
    return leasesToTake;
  }
  size_t target {0};
  if (numWorkers > numLeases) {
    target = 1;
  } else {
    target = numLeases / numWorkers;
    if (numLeases % numWorkers != 0) {
      target++;
    }
    target = std::min<size_t>(state_->params.maxLeasesToStealAtOnce.value(), target);
  }
  size_t selfCount {0};
  auto selfCountOption = util::findOption(leaseCounts, state_->params.workerID);
  if (selfCountOption.hasValue()) {
    selfCount = selfCountOption.value();
  }
  if (selfCount >= target) {
    return leasesToTake;
  }
  size_t numToTarget = target - selfCount;
  vector<lease_ptr_t> expiredCopy = expiredLeases;
  if (expiredCopy.size() > 0) {
    util::randomShuffle(expiredCopy);
    for (size_t i = 0; i < numToTarget && !expiredCopy.empty(); i++) {
      leasesToTake.push_back(expiredCopy.back());
      expiredCopy.pop_back();
    }
  } else {
    ChooseLeasesToStealOptions options;
    options.neededNum = NumberOfLeases {numToTarget};
    options.targetNum = NumberOfLeases {target};
    leasesToTake = chooseLeasesToSteal(leaseCounts, options);
  }
  return leasesToTake;
}

}} // kclpp::leases