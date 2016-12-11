#pragma once

#include <memory>
#include <vector>
#include <glog/logging.h>
#include "kclpp/TimerSettings.h"
#include "kclpp/macros/core.h"
#include "kclpp/func/Function.h"
#include "kclpp/async/EvBase.h"
#include "kclpp/async/TimerEvent.h"
#include "kclpp/async/wheel/WheelBuffer.h"
#include "kclpp/async/wheel/DynamicWheelBuffer.h"

namespace kclpp { namespace async { namespace wheel {

class HTimerWheel {
 public:
  using base_t = kclpp::async::EvBase;
  using timer_settings_t = kclpp::TimerSettings;
  using timer_t = kclpp::async::TimerEvent;
  using timer_ptr_t = std::unique_ptr<timer_t>;
  using cb_t = kclpp::func::Function<void>;
  using clock_t = std::chrono::steady_clock;
  using time_delta_t = std::chrono::milliseconds;
  using time_point_t = time_delta_t;

  struct LevelSettings {
    uint64_t resolution {0};
    uint64_t slots {0};
    LevelSettings(){}
    LevelSettings(uint64_t res, uint64_t slot): resolution(res), slots(slot){}
    LevelSettings(std::pair<uint64_t, uint64_t> numPair)
      : resolution(numPair.first), slots(numPair.second) {}
  };

  struct WheelSettings {
    using setting_vec_t = std::vector<LevelSettings>;
    setting_vec_t levelSettings;
    WheelSettings(setting_vec_t&& levels)
      : levelSettings(std::forward<setting_vec_t>(levels)) {}


    using setting_init_t = std::initializer_list<std::pair<uint64_t, uint64_t>>;
    WheelSettings(setting_init_t&& initList) {
      for (auto &elem: initList) {
        levelSettings.push_back(LevelSettings(elem));
      }
    }
    WheelSettings(){}
  };

  class Entry {
   protected:
    cb_t callback_;
    time_delta_t interval_ {0};
    time_point_t addedTime_;
    bool repeating_ {false};
   public:
    bool good() const {
      return !!callback_;
    }
    operator bool() const {
      return good();
    }
    Entry(const cb_t& cb, time_delta_t interval, bool repeating = false)
      : callback_(cb), interval_(interval), repeating_(repeating) {}

    void setAddedTime(time_point_t added) {
      addedTime_ = added;
    }

    time_delta_t getRemainingTime(time_point_t nowTime) {
      auto endTime = addedTime_ + interval_;
      if (endTime <= nowTime) {
        return time_delta_t {0};
      }
      return endTime - nowTime;
    }

    Entry(cb_t&& cb, time_delta_t interval, bool repeating = false)
      : callback_(std::forward<cb_t>(cb)), interval_(interval), repeating_(repeating) {}
    Entry(){}
    void operator()() {
      DCHECK(good());
      callback_();
    }
    time_delta_t getInterval() {
      return interval_;
    }
    bool isRepeating() const {
      return repeating_;
    }
  };

  struct Slot {
    std::vector<Entry> entries;
  };

  class Bucket {
   protected:
    bool isBottom_ {false};
    size_t resolutionMsec_ {0};
    size_t nSlots_ {0};
    DynamicWheelBuffer<Slot> buffer_;
   public:
    Bucket(){}
    Bucket(bool isBottom, size_t resolution, size_t slots)
      : isBottom_(isBottom), resolutionMsec_(resolution), nSlots_(slots) {
      buffer_ = DynamicWheelBuffer<Slot>::create(nSlots_);
    }
    time_delta_t timeCapacity() const {
      return time_delta_t {getResolution() * getNumSlots()};
    }
    size_t getResolution() const {
      return resolutionMsec_;
    }
    size_t getNumSlots() const {
      return nSlots_;
    }
    Slot& getSlotAt(size_t idx) {
      CHECK(idx < getNumSlots());
      return buffer_.getAt(idx);
    }
    size_t getSlotIdx(time_delta_t msecDelay) {
      if (msecDelay > timeCapacity()) {
        throw std::runtime_error("slot out of range.");
      }
      size_t totalMsec = msecDelay.count();
      size_t overflow = totalMsec % getResolution();
      size_t toConsider = totalMsec - overflow;
      size_t idx = 0;
      size_t current = 0;
      if (totalMsec < getResolution()) {
        idx = 0;
      } else {
        for (;;) {
          current += getResolution();
          if (overflow > 0) {
            if (current < toConsider) {
              idx++;
            } else {
              break;
            }
          } else {
            if (current <= toConsider) {
              idx++;
            } else {
              break;
            }
          }
        }
      }
      if (isBottom_) {
        if (idx == 0) {
          size_t zeroDelta = overflow;
          size_t tickDelta = getResolution() - overflow;
          if (tickDelta < zeroDelta) {
            idx++;
          }
        }
      } else if (totalMsec < getResolution()) {
        idx = 0;
      }

      return idx;
    }
    Slot& getSlot(time_delta_t delay) {
      return getSlotAt(getSlotIdx(delay));
    }
    void addEntry(time_point_t nowTime, Entry&& entry) {
      auto remaining = entry.getRemainingTime(nowTime);
      auto slotIdx = getSlotIdx(remaining);
      getSlotAt(slotIdx).entries.push_back(std::move(entry));
    }
    void advance() {
      buffer_.advance();
    }
  };

  struct WheelLevel {
    Bucket bucket;
    timer_ptr_t timer {nullptr};
    size_t tickCounter {0};
    size_t skipTick {0};

    WheelLevel(){}
    WheelLevel(Bucket&& bucket)
      : bucket(std::move(bucket)) {}

    WheelLevel(Bucket&& bucket, timer_ptr_t&& timer)
      : bucket(std::move(bucket)), timer(std::move(timer)){}
  };

 protected:
  base_t *base_ {nullptr};
  WheelSettings wheelSettings_;
  std::vector<WheelLevel> levels_;
  time_point_t cachedNow_;
  bool nowInititialized_ {false};

  void updateNowOffset() {
    auto steadyNow = clock_t::now().time_since_epoch();
    cachedNow_ = std::chrono::duration_cast<time_point_t>(steadyNow);
    nowInititialized_ = true;
  }

  time_point_t getNowOffset() {
    if (!nowInititialized_) {
      updateNowOffset();
    }
    return cachedNow_;
  }

  HTimerWheel(base_t *base, const WheelSettings& settings)
    : base_(base), wheelSettings_(settings) {}

  KCLPP_DISABLE_COPY_AND_ASSIGN(HTimerWheel);

  void addEntry(Entry&& entry) {
    auto now = getNowOffset();
    auto offset = entry.getRemainingTime(now);
    bool added = false;
    for (size_t i = 0; i < levels_.size(); i++) {
      auto& level = levels_[i];
      if (offset < level.bucket.timeCapacity()) {
        level.bucket.addEntry(now, std::move(entry));
        added = true;
        break;
      }
    }
    if (!added) {
      throw std::runtime_error {"offset out of range."};
    }
  }

  std::vector<Entry> repeatingEntriesToAdd_;
  std::vector<Entry> subEntriesToAdd_;
  std::vector<Entry> entriesToExecute_;


  void executePendingEntries() {
    for (auto& entry: entriesToExecute_) {
      Entry movedEntry = std::move(entry);
      CHECK(!!movedEntry);
      movedEntry();
      if (movedEntry.isRepeating()) {
        repeatingEntriesToAdd_.push_back(std::move(movedEntry));
      }
    }
    entriesToExecute_.clear();
  }

  void reAddRepeatingEntries() {
    for (auto &entry: repeatingEntriesToAdd_) {
      addNewEntry(std::move(entry));
    }
    repeatingEntriesToAdd_.clear();
  }

  void addPendingExecution(Entry&& entry) {
    CHECK(!!entry);
    entriesToExecute_.push_back(std::move(entry));
  }

  void addSubEntries() {
    for (auto& entry: subEntriesToAdd_) {
      addEntry(std::move(entry));
    }
    subEntriesToAdd_.clear();
  }


  using entry_vec_t = std::vector<Entry>;
  void processUpperBucketSlots(entry_vec_t& entries) {
    for (auto& entry: entries) {
      auto remaining = entry.getRemainingTime(getNowOffset());
      auto remainingCount = remaining.count();
      size_t bucket1Res = wheelSettings_.levelSettings[0].resolution;
      if (remainingCount < bucket1Res) {
        size_t zeroDelta = remainingCount;
        size_t resDelta = bucket1Res - remainingCount;
        if (zeroDelta < resDelta) {
          addPendingExecution(std::move(entry));
        }
      }
      if (entry) {
        subEntriesToAdd_.push_back(std::move(entry));
      }
    }
  }


  void processBucket1() {
    auto& current = levels_[0].bucket.getSlot(time_delta_t {0});
    for (auto& entry: current.entries) {
      if (entry) {
        addPendingExecution(std::move(entry));
      }
    }
    current.entries.clear();
  }

  void processBucketN(size_t idx) {
    CHECK(idx > 0);
    auto& current = levels_[idx].bucket.getSlot(time_delta_t {0});
    processUpperBucketSlots(current.entries);
    current.entries.clear();
  }

  void onLevelTickImpl(size_t idx) {
    updateNowOffset();
    if (idx == 0) {
      processBucket1();
      executePendingEntries();
    } else {
      processBucketN(idx);
      executePendingEntries();
      onLevelTickImpl(idx-1);
    }
  }
  void onAfterTick() {
    executePendingEntries();
    updateNowOffset();
    addSubEntries();
    reAddRepeatingEntries();
    executePendingEntries();
  }

  void advanceBuckets(size_t levelIdx) {
    for (size_t i = 0; i <= levelIdx; i++) {
      levels_[i].bucket.advance();
    }
  }

  void onLevelTick(size_t idx) {
    if (idx == (levels_.size() - 1)) {
      // this is the top level
      onLevelTickImpl(idx);
      advanceBuckets(idx);
      onAfterTick();
    } else {
      levels_[idx].tickCounter++;
      size_t counter = levels_[idx].tickCounter;
      size_t nSkip = levels_[idx].skipTick;
      if (counter == nSkip) {
        levels_[idx].tickCounter = 0;
        return;
      } else {
        onLevelTickImpl(idx);
        advanceBuckets(idx);
        onAfterTick();
      }
    }
  }


  void addNewEntry(Entry&& entry) {
    entry.setAddedTime(getNowOffset());
    addEntry(std::move(entry));
  }

  void initLevels() {
    levels_.reserve(wheelSettings_.levelSettings.size());
    size_t idx = 0;
    for (auto &setting: wheelSettings_.levelSettings) {
      bool isBottom = idx == 0;
      levels_.push_back(WheelLevel(
        Bucket(isBottom, setting.resolution, setting.slots)
      ));
    }
    size_t lastIdx = levels_.size() - 1;
    for (size_t i = 0; i < lastIdx; i++) {
      size_t parentRes = wheelSettings_.levelSettings[i+1].resolution;
      size_t selfRes = wheelSettings_.levelSettings[i].resolution;
      size_t skipTick = parentRes / selfRes;
      levels_[i].skipTick = skipTick;
    }
  }

 public:
  static HTimerWheel* createNew(base_t *base, const WheelSettings& settings) {
    auto wheel = new HTimerWheel(base, settings);
    wheel->initLevels();
    return wheel;
  }

  template<typename TCallable>
  void add(TCallable&& callable, time_delta_t msecDelay, bool repeating) {
    addNewEntry(Entry {callable, msecDelay, repeating});
  }

  template<typename TCallable>
  void add(const TCallable& callable, time_delta_t msecDelay, bool repeating) {
    addNewEntry(Entry {callable, msecDelay, repeating});
  }

  template<typename TCallable>
  void addRepeating(TCallable&& callable, time_delta_t msecInterval) {
    bool repeating = true;
    add(std::forward<TCallable>(callable), msecInterval, repeating);
  }

  template<typename TCallable>
  void addOneShot(TCallable&& callable, time_delta_t msecInterval) {
    bool repeating = false;
    add(std::forward<TCallable>(callable), msecInterval, repeating);
  }

  void start() {
    updateNowOffset();
    using msec = std::chrono::milliseconds;
    for (size_t i = 0; i < levels_.size(); i++) {
      auto setting = wheelSettings_.levelSettings[i];
      levels_[i].timer.reset(TimerEvent::createNewTimerEvent(
        base_, timer_settings_t { msec { setting.resolution} }
      ));
      levels_[i].timer->setHandler([this, i]() {
        onLevelTick(i);
      });
    }
  }

  void stop() {
    for (size_t i = 0; i < levels_.size(); i++) {
      levels_[i].timer->del();
      levels_[i].timer.reset();
    }
  }

  auto getNow() -> decltype(getNowOffset()) {
    return getNowOffset();
  }
};

}}} // kclpp::async::wheel
