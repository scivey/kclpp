#pragma once
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

class LeaseCheckpointerIf {
 public:
  using aws_errors_t = Aws::Client::AWSError<Aws::DynamoDB::DynamoDBErrors>;
  using lease_ptr_t = std::shared_ptr<Lease>;
  using ShardID = kinesis::ShardID;
 public:
  using unit_outcome_t = Outcome<Unit, aws_errors_t>;
  using unit_cb_t = func::Function<void, unit_outcome_t>;

  struct SetCheckpointParams {
    ShardID shardID;
    ExtendedSequenceNumber sequenceNumber;
    ConcurrencyToken concurrencyToken;
  };

  struct SetCheckpointResult {
    bool succeeded {false};
  };
  using set_checkpoint_outcome_t = Outcome<SetCheckpointResult, aws_errors_t>;
  using set_checkpoint_cb_t = func::Function<void, set_checkpoint_outcome_t>;

  virtual void setCheckpoint(const SetCheckpointParams&, set_checkpoint_cb_t) = 0;

  struct GetCheckpointResult {
    Optional<ExtendedSequenceNumber> sequenceNumber;
  };
  using get_checkpoint_outcome_t = Outcome<GetCheckpointResult, aws_errors_t>;
  using get_checkpoint_cb_t = std::function<void(const get_checkpoint_outcome_t&)>;
  virtual void getCheckpoint(const ShardID&, get_checkpoint_cb_t) = 0;

  virtual ~LeaseCheckpointerIf() = default;
};

}} // kclpp::leases