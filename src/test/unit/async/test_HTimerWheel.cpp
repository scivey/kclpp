#include <gtest/gtest.h>
#include <glog/logging.h>
#include <array>
#include <list>
#include <vector>
#include <chrono>
#include "kclpp/TimerSettings.h"
#include "kclpp/async/TimerEvent.h"
#include "kclpp/async/EvBase.h"
#include "kclpp/async/wheel/HTimerWheel.h"
#include "kclpp/util/misc.h"

using namespace std;
using kclpp::async::EvBase;
using kclpp::async::wheel::HTimerWheel;
using kclpp::async::TimerEvent;
using kclpp::TimerSettings;

namespace util = kclpp::util;
using msec = std::chrono::milliseconds;

class WheelTester {
 public:
  using time_offset_t = decltype(std::declval<HTimerWheel>().getNow());
 protected:
  std::shared_ptr<HTimerWheel> wheel_ {nullptr};
  uint64_t interval_ {0};
  bool noisy_ {false};
  time_offset_t prevTime_;
  std::vector<uint64_t> points_;
  void onTick() {
    auto nowTime = wheel_->getNow();
    auto delta = nowTime.count() - prevTime_.count();
    points_.push_back(delta);
    prevTime_ = nowTime;
  }
 public:
  const std::vector<uint64_t> getPoints() const {
    return points_;
  }
  WheelTester(std::shared_ptr<HTimerWheel> wheel, uint64_t interval)
    : wheel_(wheel), interval_(interval){}
  WheelTester(std::shared_ptr<HTimerWheel> wheel, uint64_t interval, bool noisy)
    : wheel_(wheel), interval_(interval), noisy_(noisy) {}
  void add() {
    prevTime_ = wheel_->getNow();
    wheel_->addRepeating([this]() { onTick(); }, std::chrono::milliseconds{ interval_});
  }
};


TEST(TestHTimerWheel, TestSanity) {
  HTimerWheel::WheelSettings wheelSettings {
    {1, 10},
    {10, 10},
    {100, 5},
    {500, 2},
    {1000, 10},
    {10000, 6}
  };
  auto base = util::createShared<EvBase>();
  auto wheel = util::createShared<HTimerWheel>(base.get(), wheelSettings);
  wheel->start();
  WheelTester tester1 {wheel, 25};
  tester1.add();
  WheelTester tester2 {wheel, 134};
  tester2.add();

  bool persist = false;
  auto timerEvent = util::createShared<TimerEvent>(base.get(), persist);
  bool finished = false;
  timerEvent->setHandler([&tester1, &tester2, &finished]() {
    auto pts1 = tester1.getPoints();
    EXPECT_TRUE(pts1.size() > 0);
    EXPECT_TRUE(pts1.size() < 10);
    auto pts2 = tester2.getPoints();
    EXPECT_EQ(1, pts2.size());
    for (auto pt: pts1) {
      EXPECT_TRUE(15 < pt);
      EXPECT_TRUE(pt < 50);
    }
    EXPECT_TRUE(pts2[0] < 150);
    EXPECT_TRUE(pts2[0] > 115);
    finished = true;
  });
  timerEvent->add(TimerSettings{chrono::milliseconds{200}});

  TimerSettings loopSettings {chrono::milliseconds {5}};
  while (!finished) {
    base->runFor(loopSettings);
  }
  EXPECT_TRUE(finished);
}
