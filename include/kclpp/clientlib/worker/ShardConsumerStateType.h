#pragma once

namespace kclpp { namespace clientlib { namespace worker {

enum class ShardConsumerStateType {
  NOT_STARTED,
  FAILED_TO_INITIALIZE,
  WAITING_ON_PARENT_SHARDS,
  INITIALIZING,
  READY_FOR_PROCESSING,
  PROCESSING,
  SHUTDOWN_REQUESTED,
  SHUTTING_DOWN,
  SHUTDOWN_COMPLETE
};

ShardConsumerStateType getInitialShardConsumerStateType() noexcept;

}}} // kclpp::clientlib::worker

