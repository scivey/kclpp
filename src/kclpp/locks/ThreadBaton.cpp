#include "kclpp/locks/ThreadBaton.h"
#include <glog/logging.h>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>

using namespace std;

namespace kclpp { namespace locks {

ThreadBaton::AlreadyPosted::AlreadyPosted()
  : std::runtime_error("AlreadyPosted") {}


void ThreadBaton::post() {
  std::unique_lock<std::mutex> lk{mutex_};
  doPost();
  lk.unlock();
  condition_.notify_one();
}

void ThreadBaton::wait() {
  if (posted_.load(std::memory_order_relaxed)) {
    // fast path
    return;
  }
  std::unique_lock<std::mutex> lk{mutex_};

  // check if producer posted between `.load()` and lock acquisition.
  // otherwise consumer could get stuck (race condition)
  if (posted_.load()) {
    return;
  }

  condition_.wait(lk, [this]() {
    return posted_.load();
  });

  for (;;) {
    if (posted_.load()) {
      break;
    }
    this_thread::sleep_for(chrono::milliseconds(10));
  }
}

bool ThreadBaton::isDone() const {
  return posted_.load();
}

// NB lock must be held (protected method)
void ThreadBaton::doPost() {
  if (posted_.load()) {
    throw AlreadyPosted();
  }
  bool expected = false;
  bool desired = true;
  if (!posted_.compare_exchange_strong(expected, desired)) {
    throw AlreadyPosted();
  }
}

}} // kclpp::locks
