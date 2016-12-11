#pragma once
#include <memory>
#include <atomic>
#include <functional>
#include <vector>

#include "kclpp/leases/Lease.h"
#include "kclpp/leases/types.h"
#include "kclpp/leases/ExtendedSequenceNumber.h"
#include "kclpp/leases/LeaseCheckpointerIf.h"
#include "kclpp/leases/LeaseCoordinator.h"
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

class LeaseCheckpointer: public LeaseCheckpointerIf {
 protected:
  using coordinator_ptr_t = std::shared_ptr<LeaseCoordinator>;
  coordinator_ptr_t leaseCoordinator_ {nullptr};
 public:
  LeaseCheckpointer(coordinator_ptr_t);
  void setCheckpoint(const SetCheckpointParams&, set_checkpoint_cb_t) override;
  void getCheckpoint(const ShardID&, get_checkpoint_cb_t) override;
};

}} // kclpp::leases