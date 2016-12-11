#pragma once

namespace kclpp { namespace clientlib { namespace worker {

enum class InitialPositionInStream {
  LATEST,
  TRIM_HORIZON
  // TODO: add AT_TIMESTAMP position
};

}}} // kclpp::clientlib::worker