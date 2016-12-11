#pragma once
#include <atomic>
#include <cstddef>

namespace kclpp {

class AsyncGroup {
 protected:
  std::atomic<size_t> counter_ {0};
  // Don't need full atomic for `target` here,
  // just a memory barrier of some kind prior
  // to launching any of the subtasks.
  // Store barrier I think?
  // For now, does the job but it's overkill
  std::atomic<size_t> target_ {0};

  std::atomic<bool> leaderChosen_ {false};
 public:
  AsyncGroup(size_t targetNum);
  bool isDone() const;
  void post();
 protected:
  bool becomeLeaderCAS();
 public:
  // separated from becomeLeaderCAS for easier debug logging
  bool becomeLeader();
};

} // kclpp
