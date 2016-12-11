#pragma once
#include <set>
#include <memory>
#include <utility>

#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/core/utils/Outcome.h>

#include <aws/core/utils/memory/stl/AWSVector.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <aws/dynamodb/model/AttributeValue.h>

#include "kclpp/Optional.h"
#include "kclpp/leases/types.h"
#include "kclpp/leases/ExtendedSequenceNumber.h"
#include "kclpp/leases/errors.h"
#include "kclpp/kinesis/types.h"
#include "kclpp/clock/Clock.h"
#include "kclpp/util/util.h"


namespace kclpp { namespace leases {

class Lease: public std::enable_shared_from_this<Lease> {
 public:
  using NanosecondDuration = clock::NanosecondDuration;
  using NanosecondPoint = clock::NanosecondPoint;
  using AttributeValue = Aws::DynamoDB::Model::AttributeValue;

  struct LeaseState {
    LeaseKey leaseKey;
    Optional<LeaseOwner> leaseOwner;
    Optional<ConcurrencyToken> concurrencyToken;
    Optional<NanosecondPoint> lastCounterIncrementNanoseconds;
    Optional<LeaseCounter> leaseCounter;
    Optional<CheckpointID> checkpointID;
    Optional<SubCheckpointID> subCheckpointID;
    Optional<NumberOfLeaseSwitches> ownerSwitchesSinceCheckpoint;
    std::set<kinesis::ShardID> parentShardIDs;
  };
  static NanosecondDuration getMaxLeaseAge();

 protected:
  LeaseState leaseState_;
 public:
  Lease(LeaseState&& initialState);
  Lease();

  const LeaseState& getState() const;
  bool isExpired(NanosecondDuration leaseDuration, NanosecondPoint asOf) const;

  using dynamo_dict_t = Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue>;
  using from_dynamo_outcome_t = Aws::Utils::Outcome<Lease, DeserializationError>;

  static from_dynamo_outcome_t fromDynamoDict(const dynamo_dict_t& dynamoDict);
  dynamo_dict_t toDynamoDict() const;

  Optional<ExtendedSequenceNumber> getExtendedSequenceNumber() const;
  Lease copyWithNewConcurrencyToken() const;
  std::shared_ptr<Lease> copySharedWithNewConcurrencyToken() const;
  std::shared_ptr<Lease> copyShared() const;
  std::shared_ptr<Lease> copySharedWithIncrementedCounter() const;
};

}} // kclpp::leases
