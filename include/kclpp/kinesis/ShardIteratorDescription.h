#pragma once
#include <aws/kinesis/KinesisClient.h>
#include <aws/kinesis/model/PutRecordsRequest.h>
#include <aws/kinesis/model/PutRecordsResult.h>
#include <aws/kinesis/model/GetRecordsRequest.h>
#include <aws/kinesis/model/GetRecordsResult.h>
#include <aws/kinesis/model/DescribeStreamRequest.h>
#include <aws/kinesis/model/DescribeStreamResult.h>
#include <aws/kinesis/model/ListStreamsRequest.h>
#include <aws/kinesis/model/ListStreamsResult.h>
#include <aws/kinesis/model/GetShardIteratorRequest.h>
#include <aws/kinesis/model/GetShardIteratorResult.h>
#include <aws/kinesis/model/ShardIteratorType.h>
#include "kclpp/kinesis/types.h"
#include "kclpp/kinesis/ShardIteratorProperties.h"

namespace kclpp { namespace kinesis {

class ShardIteratorDescription {
 protected:
  ShardIteratorID iteratorID_;
  ShardIteratorProperties properties_;
 public:
  const ShardIteratorProperties& getProperties() const;
  const ShardIteratorID& getIteratorID() const;
  ShardIteratorDescription();
  ShardIteratorDescription(ShardIteratorID iterID, ShardIteratorProperties&& properties);
};

}} // kclpp::kinesis
