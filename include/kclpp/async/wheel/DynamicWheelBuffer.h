#pragma once
#include <vector>
#include <glog/logging.h>
#include "kclpp/macros/core.h"

namespace kclpp { namespace async { namespace wheel {

template<typename T>
class FixedVector {
 public:
  using vector_t = std::vector<T>;
 protected:
  size_t size_ {0};
  vector_t vec_;
  FixedVector(size_t size): size_(size){}
  void initialize() {
    CHECK(vec_.empty());
    vec_.reserve(size_);
    for (size_t i = 0; i < size_; i++) {
      vec_.emplace_back();
    }
  }
 public:
  FixedVector(){}
  static FixedVector create(size_t n) {
    FixedVector instance {n};
    instance.initialize();
    return instance;
  }
  T& at(size_t idx) {
    DCHECK(idx < size_);
    return vec_.at(idx);
  }
  const T& at(size_t idx) const {
    DCHECK(idx < size_);
    return vec_.at(idx);
  }
  size_t size() const {
    return vec_.size();
  }
};


template<typename T>
class DynamicWheelBuffer {
 public:
  using value_type = T;
 protected:
  KCLPP_DISABLE_COPY_AND_ASSIGN(DynamicWheelBuffer);
  FixedVector<T> buff_;
  size_t currentIndex_ {0};
  size_t getRealIndex(size_t idx) const {
    return (currentIndex_ + idx) % buff_.size();
  }
  DynamicWheelBuffer(FixedVector<T> &&buff): buff_(std::move(buff)) {}

 public:
  DynamicWheelBuffer(DynamicWheelBuffer&& other)
    : buff_(std::move(other.buff_)), currentIndex_(other.currentIndex_) {
    other.currentIndex_ = 0;
  }
  DynamicWheelBuffer& operator=(DynamicWheelBuffer&& other) {
    std::swap(buff_, other.buff_);
    std::swap(currentIndex_, other.currentIndex_);
    return *this;
  }
  DynamicWheelBuffer(){}
  bool good() const {
    return !buff_.empty();
  }
  operator bool() const {
    return good();
  }
  static DynamicWheelBuffer create(size_t nItems) {
    return DynamicWheelBuffer(FixedVector<T>::create(nItems));
  }
  void advance() {
    currentIndex_ = (currentIndex_ + 1) % buff_.size();
  }
  T& getAt(size_t idx) {
    return buff_.at(getRealIndex(idx));
  }
  void setAt(size_t idx, T&& elem) {
    getAt(idx) = std::move(elem);
  }
};

}}} // kclpp::async::wheel