#include "kclpp/clientlib/worker/ShardConsumerStateType.h"

namespace kclpp { namespace clientlib { namespace worker {

ShardConsumerStateType getInitialShardConsumerStateType() noexcept {
  return ShardConsumerStateType::NOT_STARTED;
}

}}} // kclpp::clientlib::worker

