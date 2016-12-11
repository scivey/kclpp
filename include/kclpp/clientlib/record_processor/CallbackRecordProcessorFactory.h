#pragma once
#include <memory>
#include "kclpp/clientlib/record_processor/RecordProcessor.h"
#include "kclpp/func/Function.h"

namespace kclpp { namespace clientlib { namespace record_processor {

class CallbackRecordProcessorFactory: public RecordProcessorFactory {
 public:
  using callback_t = func::Function<std::shared_ptr<RecordProcessor>>;
 protected:
  callback_t callback_;
 public:
  CallbackRecordProcessorFactory(callback_t callback): callback_(callback){}
  std::shared_ptr<RecordProcessor> build() override {
    return callback_();
  }
};

}}} // kclpp::clientlib::record_processor
