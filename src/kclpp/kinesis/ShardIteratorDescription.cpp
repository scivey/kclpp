#include "kclpp/kinesis/ShardIteratorDescription.h"

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

ShardIteratorDescription::ShardIteratorDescription(){}

ShardIteratorDescription::ShardIteratorDescription(ShardIteratorID iterID,
    ShardIteratorProperties&& properties)
  : iteratorID_(iterID), properties_(std::forward<ShardIteratorProperties>(properties)) {}

const ShardIteratorProperties& ShardIteratorDescription::getProperties() const {
  return properties_;
}

const ShardIteratorID& ShardIteratorDescription::getIteratorID() const {
  return iteratorID_;
}

}} // kclpp::kinesis
