#pragma once

#include "kclpp_test_support/LeaseManagerTestContext.h"
#include "kclpp/UniqueToken.h"

namespace kclpp_test_support {

class TableGuard {
 protected:
  LeaseManagerTestContext *testContext_ {nullptr};
  kclpp::UniqueToken token_;
  TableGuard(LeaseManagerTestContext *ctx);
 public:
  TableGuard(TableGuard&& other);
  TableGuard& operator=(TableGuard&& other);
  static TableGuard create(LeaseManagerTestContext *ctx);
  ~TableGuard();
};

} // kclpp_test_support