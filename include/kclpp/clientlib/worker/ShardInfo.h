#pragma once

#include <set>
#include <chrono>

#include "kclpp/leases/ExtendedSequenceNumber.h"
#include "kclpp/leases/types.h"
#include "kclpp/kinesis/types.h"

namespace kclpp { namespace clientlib { namespace worker {

class ShardInfo {
 public:
  struct ShardInfoState {
    kinesis::StreamName streamName;
    kinesis::ShardID shardID;
    leases::ConcurrencyToken concurrencyToken;
    std::set<kinesis::ShardID> parentShardIDs;
    leases::ExtendedSequenceNumber currentCheckpoint;
  };
 protected:
  ShardInfoState state_;
 public:
  ShardInfo(){}
  ShardInfo(ShardInfoState&& initialState)
    : state_(std::forward<ShardInfoState>(initialState)) {}

  const ShardInfoState& getState() const noexcept {
    return state_;
  }

  bool isCompleted() const noexcept {
    return false;
  }
};

}}} // kclpp::clientlib::worker