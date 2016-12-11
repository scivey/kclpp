#include "kclpp/AsyncGroup.h"
#include <atomic>

namespace kclpp {

AsyncGroup::AsyncGroup(size_t targetNum) {
  target_.store(targetNum);
}

bool AsyncGroup::isDone() const {
  return counter_.load() == target_.load();
}

void AsyncGroup::post() {
  counter_.fetch_add(1);
}

bool AsyncGroup::becomeLeaderCAS() {
  for (;;) {
    bool expected = leaderChosen_.load();
    if (expected) {
      return false;
    }
    bool desired = true;
    // not sure if a spurious failure here is possible.
    // with c++11's default memory model on x64,
    // I think it isn't.
    // Still, CAS loop just in case.
    if (leaderChosen_.compare_exchange_strong(expected, desired)) {
      return true;
    }
  }    
}

bool AsyncGroup::becomeLeader() {
  auto result = becomeLeaderCAS();
  return result;
}

} // kclpp
