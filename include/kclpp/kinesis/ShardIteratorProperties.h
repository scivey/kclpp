#pragma once
#include <aws/kinesis/model/ShardIteratorType.h>
#include "kclpp/kinesis/types.h"

namespace kclpp { namespace kinesis {

/*
  ShardIteratorProperties contains shard iterator attributes which don't change
  during iteration. (as opposed mainly to ShardIteratorID, which does.)
*/

struct ShardIteratorProperties {
  using ShardIteratorType = Aws::Kinesis::Model::ShardIteratorType;  
  StreamName streamName;
  ShardID shardID;
  ShardIteratorType iteratorType {ShardIteratorType::NOT_SET};  
};

}} // kclpp::kinesis
