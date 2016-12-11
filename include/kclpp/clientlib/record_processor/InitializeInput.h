#pragma once
#include "kclpp/kinesis/types.h"

namespace kclpp { namespace clientlib { namespace record_processor {

struct InitializeInput {
  kinesis::ShardID shardID;
};

}}} // kclpp::clientlib::record_processor
