#pragma once
#include <array>

namespace kclpp { namespace async { namespace wheel {

template<typename T, size_t NItems>
class WheelBuffer {
 public:
  using value_type = T;
  static const size_t N = NItems;
  using array_t = std::array<T, NItems>;
 protected:
  array_t buff_;
  size_t currentIndex_ {0};
  size_t getRealIndex(size_t idx) {
    return (currentIndex_ + idx) % NItems;
  }
 public:
  void advance() {
    currentIndex_ = (currentIndex_ + 1) % NItems;
  }
  T& getAt(size_t idx) {
    return buff_[getRealIndex(idx)];
  }
  void setAt(size_t idx, T&& elem) {
    getAt(idx) = std::move(elem);
  }
};

}}} // kclpp::async::wheel
