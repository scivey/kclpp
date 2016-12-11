#pragma once

namespace kclpp { namespace clientlib { namespace worker {

enum class WorkerTaskType {
  BLOCK_ON_PARENT_SHARD,
  INITIALIZE,
  PROCESS,
  SHUTDOWN,
  SHUTDOWN_NOTIFICATION,
  SHUTDOWN_COMPLETE,
  SHARDSYNC
};

}}} // kclpp::clientlib::worker