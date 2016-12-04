#pragma once

#include <glog/logging.h>

// this isn't used in library code - just for testing
#define KCLPP_CHECK_OUTCOME(outcome) \
  do { \
    if (!outcome.IsSuccess()) { \
      auto err = outcome.GetError(); \
      LOG(INFO) << err.GetExceptionName() << " : '" << err.GetMessage() << "'"; \
      throw err; \
    } \
  } while(0)
