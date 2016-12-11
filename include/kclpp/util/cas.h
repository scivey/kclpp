#pragma once

#include <atomic>

namespace kclpp { namespace util {

template<typename T>
bool casLoop(std::atomic<T>& atomVar, T expected, T desired) {
  for (;;) {
    auto current = atomVar.load(std::memory_order_relaxed);
    if (current != expected) {
      return false;
    }
    if (atomVar.compare_exchange_strong(current, desired)) {
      return true;
    }
  }  
}

}} // kclpp::util