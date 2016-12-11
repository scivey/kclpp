#include "kclpp/leases/LeaseCheckpointer.h"
#include <memory>
#include <atomic>
#include <functional>
#include <vector>

#include "kclpp/leases/Lease.h"
#include "kclpp/leases/types.h"
#include "kclpp/leases/ExtendedSequenceNumber.h"
#include "kclpp/Optional.h"
#include "kclpp/leases/LeaseManager.h"
#include "kclpp/kinesis/types.h"
#include "kclpp/leases/LeaseRenewer.h"
#include "kclpp/leases/LeaseTaker.h"
#include "kclpp/clock/Clock.h"
#include "kclpp/leases/LeaseManager.h"
#include "kclpp/locks/Synchronized.h"
#include "kclpp/AsyncGroup.h"
#include "kclpp/async/KCLAsyncContext.h"
#include "kclpp/util/cas.h"

namespace kclpp { namespace leases {

LeaseCheckpointer::LeaseCheckpointer(coordinator_ptr_t coordinator): leaseCoordinator_(coordinator) {}

void LeaseCheckpointer::setCheckpoint(const SetCheckpointParams& params, set_checkpoint_cb_t callback) {
  LeaseKey leaseKey{params.shardID.value()};
  auto existingLease = leaseCoordinator_->getCurrentlyHeldLease(leaseKey);
  if (!existingLease.hasValue()) {
    LOG(INFO) << "not updating checkpoint for non-owned lease: '" << leaseKey.value() << "'";
    SetCheckpointResult result;
    result.succeeded = false;
    callback(set_checkpoint_outcome_t{std::move(result)});
    return;
  }
  Lease::LeaseState newState = existingLease.value()->getState();
  newState.checkpointID.assign(params.sequenceNumber.sequenceNumber());
  newState.subCheckpointID = params.sequenceNumber.subSequenceNumber();
  auto newLease = std::make_shared<Lease>(std::move(newState));
  leaseCoordinator_->updateLease(newLease, params.concurrencyToken,
    [callback, this](const auto& outcome) {
      if (!outcome.IsSuccess()) {
        callback(set_checkpoint_outcome_t{outcome.GetError()});
        return;
      }
      const auto& result = outcome.GetResult();
      SetCheckpointResult checkResult;
      checkResult.succeeded = result.updated;
      callback(set_checkpoint_outcome_t{std::move(checkResult)});
    }
  );
}

void LeaseCheckpointer::getCheckpoint(const ShardID& shardID, get_checkpoint_cb_t callback) {
  LeaseKey leaseKey {shardID.value()};
  leaseCoordinator_->getLeaseManager()->getLease(leaseKey, [this, callback](const auto& outcome) {
    if (!outcome.IsSuccess()) {
      callback(get_checkpoint_outcome_t{outcome.GetError()});
    }
    auto mgrResult = outcome.GetResult();
    DCHECK(!!mgrResult.lease);
    GetCheckpointResult clientResult;
    clientResult.sequenceNumber = mgrResult.lease->getExtendedSequenceNumber();
    callback(get_checkpoint_outcome_t{std::move(clientResult)});
  });
}

}} // kclpp::leases