#pragma once

#include <aws/kinesis/model/Record.h>
#include "kclpp/clientlib/RecordProcessorCheckpointerIf.h"

namespace kclpp { namespace clientlib { namespace record_processor {

struct ProcessRecordsInput {
  using Record = Aws::Kinesis::Model::Record;
  Aws::Vector<Record> records;
  std::shared_ptr<RecordProcessorCheckpointerIf> checkpointer;
};

}}} // kclpp::clientlib::record_processor
