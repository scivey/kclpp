#pragma once
#include <memory>
#include "kclpp/clientlib/record_processor/RecordProcessor.h"

namespace kclpp { namespace clientlib { namespace record_processor {

class RecordProcessorFactory {
 public:
  virtual std::shared_ptr<RecordProcessor> build() = 0;
  virtual ~RecordProcessorFactory() = default;
};

}}} // kclpp::clientlib::record_processor
