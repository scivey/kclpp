#pragma once
#include "kclpp/clientlib/record_processor/InitializeInput.h"
#include "kclpp/clientlib/record_processor/ProcessRecordsInput.h"
#include "kclpp/clientlib/record_processor/ShutdownInput.h"
#include "kclpp/clientlib/record_processor/RecordProcessorError.h"
#include "kclpp/func/Function.h"
#include "kclpp/Outcome.h"
#include "kclpp/Unit.h"

namespace kclpp { namespace clientlib { namespace record_processor {

class RecordProcessor {
 public:
  using unit_outcome_t = Outcome<Unit, RecordProcessorError>;
  using unit_cb_t = func::Function<void, unit_outcome_t>;
  virtual void initialize(InitializeInput&&, unit_cb_t) = 0;
  virtual void processRecords(ProcessRecordsInput&&, unit_cb_t) = 0;
  virtual void shutdown(ShutdownInput&&, unit_cb_t) = 0;
  virtual ~RecordProcessor() = default;
};

}}} // kclpp::clientlib::record_processor
