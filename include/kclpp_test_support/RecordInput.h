#pragma once
#include <string>
#include <aws/core/utils/Array.h>
#include <aws/kinesis/model/PutRecordsRequestEntry.h>

namespace kclpp_test_support {

struct RecordInput {
  using PutRecordsRequestEntry = Aws::Kinesis::Model::PutRecordsRequestEntry;
  Aws::Utils::ByteBuffer dataBuff;
  std::string partitionKey;
  RecordInput(const std::string& partKey, const std::string& data);
  PutRecordsRequestEntry toRequestEntry();
};

} // kclpp_test_support
