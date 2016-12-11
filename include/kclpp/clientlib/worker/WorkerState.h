#pragma once

#include <atomic>
#include "kclpp/util/cas.h"

namespace kclpp { namespace clientlib { namespace worker {



class WorkerShutdownState {
 protected:
  std::atomic<bool> shutdown_ {false};
  std::chrono::milliseconds shutdownStartTime_ {0};
  std::atomic<bool> shutdownComplete_ {false};
 public:
  bool setShutdown() {
    return util::casLoop(shutdown_, false, true);
  }

  bool isShutdownStarted() {
    return shutdown_.load();
  }

  bool isShutdownComplete() {
    return shutdownComplete_.load();
  }

  bool setStartTime(std::chrono::milliseconds startTime) {
    shutdownStartTime_ = startTime;
    return true;
  }

  bool setShutdownComplete() {
    return util::casLoop(shutdownComplete_, false, true);
  }
};

struct WorkerState {
  WorkerShutdownState shutdownState;
};

}}} // kclpp::clientlib::worker
