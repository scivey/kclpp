#include <gtest/gtest.h>
#include <sstream>
#include <memory>
#include <string>
#include "kclpp/func/Function.h"

namespace func = kclpp::func;
using namespace std;

std::string stringOfInt(int x) {
  std::ostringstream oss;
  oss << x;
  return oss.str();
}

using ITSFunc = func::Function<string, int>;

TEST(TestFunction, TestGoodness1) {
  ITSFunc fn;
  EXPECT_FALSE(fn.good());
  EXPECT_TRUE(!fn);
  fn = [](int x) {
    return "sure";
  };
  EXPECT_TRUE(fn.good());
  EXPECT_FALSE(!fn);
}

TEST(TestFunction, TestGoodness2) {
  ITSFunc fn([](int x) {
    return "yeah";
  });
  EXPECT_TRUE(fn.good());
  EXPECT_FALSE(!fn);
}

TEST(TestFunction, TestLambda1) {
  ITSFunc fn([](int x) {
    return stringOfInt(x);
  });
  EXPECT_EQ("7", fn(7));
}


class IntConverter {
 protected:
  size_t id_ {0};
 public:
  IntConverter(size_t id): id_(id){}
  size_t getId() const {
    return id_;
  }
  std::string convert(int x) {
    return stringOfInt(x);
  }
};

TEST(TestFunction, TestCapturingLambda) {
  auto converter = std::make_shared<IntConverter>(26);
  EXPECT_EQ(26, converter->getId());
  ITSFunc fn([converter](int x) {
    EXPECT_EQ(26, converter->getId());
    return converter->convert(x);
  });
  EXPECT_EQ("7", fn(7));
}

TEST(TestFunction, TestFreeFunc) {
  ITSFunc fn(stringOfInt);
  EXPECT_EQ("7", fn(7));
}


TEST(TestFunction, TestCopied) {
  auto lamb = [](int x) -> int {
    return x + 10;
  };
  using fn_t = func::Function<int, int>;
  fn_t func1 {lamb};
  EXPECT_EQ(17, func1(7));
  fn_t func2 {func1};
  EXPECT_EQ(15, func2(5));
  EXPECT_EQ(14, func1(4));
}

TEST(TestFunction, TestMoved) {
  auto lamb = [](int x) -> int {
    return x + 10;
  };
  using fn_t = func::Function<int, int>;
  fn_t func1 {lamb};
  EXPECT_EQ(17, func1(7));
  fn_t func2 {std::move(func1)};
  EXPECT_TRUE(func2.good());
  EXPECT_FALSE(func1.good());
  EXPECT_EQ(15, func2(5));
  EXPECT_FALSE(func1.callOption(9).hasValue());
  EXPECT_EQ(18, func2.callOption(8).value());
}

TEST(TestFunction, TestCopyLambda1) {
  auto sharedStr = std::make_shared<string>("yeah");
  auto lamb = [sharedStr]() {
    string result = *sharedStr + "-appended";
    return result;
  };
  func::Function<string> fn {lamb};
  auto result = fn();
  EXPECT_EQ("yeah-appended", result);
}

TEST(TestFunction, TestMoveLambdaAssumptions) {
  auto strPtr = std::make_unique<string>("yeah");
  auto lamb = [ownedStr = std::move(strPtr)]() {
    string result = *ownedStr + "-appended";
    return result;
  };
  auto result = lamb();
  EXPECT_EQ("yeah-appended", result);
}

// TEST(TestFunction, TestMoveLambda1) {
//   auto strPtr = std::make_unique<string>("yeah");
//   auto lamb = [ownedStr = std::move(strPtr)]() mutable {
//     string result = *ownedStr + "-appended";
//     return result;
//   };
//   func::Function<string> fn {std::move(lamb)};
//   auto result = fn();
//   EXPECT_EQ("yeah-appended", result);
// }

