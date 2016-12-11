#include <gtest/gtest.h>
#include "kclpp/async/EventFD.h"

using namespace kclpp;
using namespace kclpp::async;
using namespace std;

TEST(TestEventFD, TestSanity1) {
  auto efd = std::move(EventFD::create().GetResult().value());
  EXPECT_TRUE(efd.getFDNum().GetResult() > 0);
  EXPECT_TRUE(efd.write(17).IsSuccess());
  EXPECT_EQ(17, efd.read().GetResult());
}

TEST(TestEventFD, TestInvalidAfterMove) {
  auto efd = std::move(EventFD::create().GetResult().value());
  EXPECT_TRUE(efd.getFDNum().GetResult() > 0);
  EXPECT_TRUE(efd.write(17).IsSuccess());
  EXPECT_TRUE(efd.read().IsSuccess());

  EventFD usurper {std::move(efd)};

  EXPECT_FALSE(efd.getFDNum().IsSuccess());
  EXPECT_TRUE(usurper.getFDNum().IsSuccess());
  EXPECT_FALSE(efd.write(10).IsSuccess());
  EXPECT_FALSE(efd.read().IsSuccess());

  EXPECT_TRUE(usurper.write(216).IsSuccess());
  EXPECT_EQ(216, usurper.read().GetResult());
}
