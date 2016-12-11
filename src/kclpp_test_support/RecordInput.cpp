#include "kclpp_test_support/RecordInput.h"
#include <aws/core/utils/Array.h>
#include <aws/kinesis/model/PutRecordsRequestEntry.h>

using namespace std;
using Aws::Kinesis::Model::PutRecordsRequestEntry;

namespace kclpp_test_support {

RecordInput::RecordInput(const string& partKey, const string& data) {
  partitionKey = partKey;
  dataBuff = Aws::Utils::ByteBuffer {
    (unsigned char*) data.c_str(), data.size()
  };
}

PutRecordsRequestEntry RecordInput::toRequestEntry() {
  PutRecordsRequestEntry entry;
  entry.SetData(dataBuff);
  entry.SetPartitionKey(partitionKey);
  return entry;
}

} // kclpp_test_support
