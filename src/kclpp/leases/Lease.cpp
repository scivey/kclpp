#include "kclpp/leases/Lease.h"
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
#include "kclpp/leases/errors.h"
#include "kclpp/kinesis/types.h"
#include "kclpp/clock/Clock.h"
#include "kclpp/util/util.h"


namespace kclpp { namespace leases {

using kclpp::clock::NanosecondPoint;
using kclpp::clock::NanosecondDuration;
using from_dynamo_outcome_t = Lease::from_dynamo_outcome_t;
using dynamo_dict_t = Lease::dynamo_dict_t;
using LeaseState = Lease::LeaseState;

NanosecondDuration Lease::getMaxLeaseAge() {
  using duration_t = typename NanosecondDuration::inner_type;
  duration_t secsPerDay = 60 * 60 * 24;
  duration_t nDays = 365;
  duration_t nanosPerSec = 1000000000;
  duration_t result = secsPerDay * nDays * nanosPerSec; // 365 days
  return NanosecondDuration {result};
}

Lease::Lease(LeaseState&& initialState)
  : leaseState_(std::forward<LeaseState>(initialState)) {}
Lease::Lease(){}

const LeaseState& Lease::getState() const {
  return leaseState_;
}

bool Lease::isExpired(NanosecondDuration leaseDuration, NanosecondPoint asOf) const {
  if (!leaseState_.lastCounterIncrementNanoseconds.hasValue()) {
    return true;
  }
  using duration_t = typename NanosecondDuration::inner_type;
  duration_t age = asOf.value() - leaseState_.lastCounterIncrementNanoseconds.value().value();
  auto maxAge = Lease::getMaxLeaseAge();
  if (age > maxAge.value()) {
    return true;
  }
  return age > leaseDuration.value();
}


from_dynamo_outcome_t Lease::fromDynamoDict(const dynamo_dict_t& dynamoDict) {
  LeaseState initialState;
  {
    auto found = dynamoDict.find("leaseKey");
    if (found == dynamoDict.end()) {
      return from_dynamo_outcome_t {DeserializationError("Missing 'leaseKey' attribute.")};
    }
    if (found->second.GetS().empty()) {
      return from_dynamo_outcome_t {DeserializationError("non-string-valued 'leaseKey'")};
    }
    initialState.leaseKey = LeaseKey {found->second.GetS()};
  }
  {
    auto found = dynamoDict.find("leaseOwner");
    if (found == dynamoDict.end()) {
      return from_dynamo_outcome_t {DeserializationError("missing 'leaseOwner'")};
    }
    if (!found->second.GetS().empty()) {
      initialState.leaseOwner.assign( LeaseOwner { found->second.GetS() } );
    }
  }
  {
    auto found = dynamoDict.find("checkpoint");
    if (found != dynamoDict.end()) {
      initialState.checkpointID.assign(
        CheckpointID { found->second.GetS() }
      );
    }
  }
  {
    auto found = dynamoDict.find("checkpointSubSequenceNumber");
    if (found != dynamoDict.end()) {
      initialState.subCheckpointID.assign(
        SubCheckpointID{ found->second.GetS() }
      );
    }
  }
  {
    auto found = dynamoDict.find("leaseCounter");
    if (found == dynamoDict.end()) {
      initialState.leaseCounter.assign( LeaseCounter{0} );
    } else {
      auto numStr = found->second.GetN();
      if (!numStr.empty()) {
        auto converted = util::safeStrToUll(numStr);
        if (!converted.IsSuccess()) {
          return from_dynamo_outcome_t {DeserializationError{"couldn't read leaseCounter as int: " + numStr}};
        }
        initialState.leaseCounter.assign( LeaseCounter{converted.GetResult()});
      }
    }
  }
  {
    auto found = dynamoDict.find("ownerSwitchesSinceCheckpoint");
    if (found == dynamoDict.end()) {
      initialState.ownerSwitchesSinceCheckpoint.assign( NumberOfLeaseSwitches{0} );
    } else {
      auto numStr = found->second.GetN();
      if (!numStr.empty()) {
        auto converted = util::safeStrToUll(numStr);
        if (!converted.IsSuccess()) {
          return from_dynamo_outcome_t {DeserializationError{
            "couldn't read ownerSwitchesSinceCheckpoint as int: " + numStr
          }};
        }
        initialState.ownerSwitchesSinceCheckpoint.assign(
           NumberOfLeaseSwitches {converted.GetResult()}
        );
      }      
    }
  }
  {
    auto found = dynamoDict.find("lastCounterIncrementNanoseconds");
    if (found == dynamoDict.end()) {
      initialState.lastCounterIncrementNanoseconds.assign( NanosecondPoint{0} );
    } else {
      auto numStr = found->second.GetN();
      if (!numStr.empty()) {
        auto converted = util::safeStrToUll(numStr);
        if (!converted.IsSuccess()) {
          return from_dynamo_outcome_t {DeserializationError{
            "couldn't read lastCounterIncrementNanoseconds as int: " + numStr
          }};
        }
        initialState.lastCounterIncrementNanoseconds.assign(
          NanosecondPoint {converted.GetResult()}
        );
      }
    }
  }
  return Lease {std::move(initialState)};
}

dynamo_dict_t Lease::toDynamoDict() const {
  dynamo_dict_t result {
    {"leaseKey", AttributeValue().SetS(leaseState_.leaseKey.value())}      
  };
  if (leaseState_.leaseOwner.hasValue()) {
    result.insert(std::make_pair(
      "leaseOwner",
      AttributeValue().SetS(leaseState_.leaseOwner.value().value())
    ));
  }
  if (leaseState_.checkpointID.hasValue()) {
    result.insert(std::make_pair(
      "checkpoint",
      AttributeValue().SetS(leaseState_.checkpointID.value().value())
    ));
  }
  if (leaseState_.subCheckpointID.hasValue()) {
    result.insert(std::make_pair(
      "checkpointSubSequenceNumber",
      AttributeValue().SetS(leaseState_.subCheckpointID.value().value())
    ));
  }
  std::string leaseCounter {"0"};
  if (leaseState_.leaseCounter.hasValue()) {
    std::ostringstream oss;
    oss << leaseState_.leaseCounter.value().value();
    leaseCounter = oss.str();
  }
  result.insert(std::make_pair(
    "leaseCounter",
    AttributeValue().SetN(leaseCounter)
  ));

  std::string ownerSwitches {"0"};
  if (leaseState_.ownerSwitchesSinceCheckpoint.hasValue()) {
    std::ostringstream oss;
    oss << leaseState_.ownerSwitchesSinceCheckpoint.value().value();
    ownerSwitches = oss.str();
  }
  result.insert(std::make_pair(
    "ownerSwitchesSinceCheckpoint",
    AttributeValue().SetN(ownerSwitches)
  ));

  std::string lastIncrement {"0"};
  if (leaseState_.lastCounterIncrementNanoseconds.hasValue()) {
    std::ostringstream oss;
    oss << leaseState_.lastCounterIncrementNanoseconds.value().value();
    lastIncrement = oss.str();
  }
  result.insert(std::make_pair(
    "lastCounterIncrementNanoseconds",
    AttributeValue().SetN(lastIncrement)
  ));
  return result;
}

Lease Lease::copyWithNewConcurrencyToken() const {
  LeaseState copiedState = leaseState_;
  return Lease {std::move(copiedState)};
}

std::shared_ptr<Lease> Lease::copySharedWithNewConcurrencyToken() const {
  return std::shared_ptr<Lease>{new Lease {copyWithNewConcurrencyToken()}};
}

std::shared_ptr<Lease> Lease::copyShared() const {
  LeaseState copiedState = leaseState_;
  return std::shared_ptr<Lease>{new Lease{std::move(copiedState)}};
}

std::shared_ptr<Lease> Lease::copySharedWithIncrementedCounter() const {
  LeaseState copiedState = leaseState_;
  if (copiedState.leaseCounter.hasValue()) {
    copiedState.leaseCounter.assign( LeaseCounter{1 + copiedState.leaseCounter.value().value() });
  } else {
    copiedState.leaseCounter.assign( LeaseCounter{1} );
  }
  return std::shared_ptr<Lease>{new Lease{std::move(copiedState)}};
}

Optional<ExtendedSequenceNumber> Lease::getExtendedSequenceNumber() const {
  Optional<ExtendedSequenceNumber> result;
  if (leaseState_.checkpointID.hasValue()) {
    if (leaseState_.subCheckpointID.hasValue()) {
      result.assign(ExtendedSequenceNumber{
        leaseState_.checkpointID.value(),
        leaseState_.subCheckpointID.value()
      });
    } else {
      result.assign(ExtendedSequenceNumber{
        leaseState_.checkpointID.value()
      });
    }
  }
  return result;
}

}} // kclpp::leases
