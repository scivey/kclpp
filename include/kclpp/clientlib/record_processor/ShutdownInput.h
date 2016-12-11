#pragma once
#include "kclpp/clientlib/ShutdownReason.h"

namespace kclpp { namespace clientlib { namespace record_processor {

struct ShutdownInput {
  ShutdownReason reason {ShutdownReason::TERMINATE};
};

}}} // kclpp::clientlib::record_processor
