#pragma once
#include <mutex>
#include <atomic>
#include <thread>

namespace kclpp { namespace locks {

// copied over from score; original concept from folly

template<typename T>
class Synchronized {
 protected:
  T instance_;
  std::mutex mutex_;
 public:
  Synchronized(const Synchronized &other) = delete;
  Synchronized& operator=(const Synchronized &other) = delete;
 public:
  Synchronized() {}
  Synchronized(Synchronized&&) = default;
  Synchronized& operator=(Synchronized&&) = default;
  Synchronized(const T& val): instance_(val){}
  Synchronized(T &&val): instance_(std::move(val)) {}

  template<typename U,
    typename = typename std::enable_if<std::is_integral<U>::value, U>::type>
  Synchronized(U arg)
    : instance_(arg){}

  template<typename U, typename... Args>
  Synchronized(typename std::enable_if<!std::is_integral<U>::value, U>::type &&arg1, Args&& ...args)
    : instance_(std::forward<U>(arg1), std::forward<Args>(args)...) {}

  class PtrHandle {
   protected:
    std::atomic<Synchronized*> parent_ {nullptr};
    PtrHandle& operator=(const PtrHandle &other) = delete;
    PtrHandle(const PtrHandle& other) = delete;
    PtrHandle(Synchronized *parent) {
      parent_.store(parent);
      parent->mutex_.lock();
    }

    friend class Synchronized;
    void maybeClose() {
      auto parentPtr = parent_.load();
      if (parentPtr) {
        if (!parent_.compare_exchange_strong(parentPtr, nullptr)) {
          return;
        }
        parentPtr->mutex_.unlock();
      }
    }
   public:
    PtrHandle(PtrHandle &&other) {
      parent_.store(other.parent_);
      other.parent_.store(nullptr);
    }
    operator T* () {
      return parent_.load()->getInstancePtr();
    }
    T* get() {
      return parent_.load()->getInstancePtr();
    }
    T& operator*() {
      T* ptr = get();
      return *ptr;
    }
    PtrHandle& operator=(PtrHandle &&other) {
      maybeClose();
      for (;;) {
        auto otherParent = other.parent_.load();
        if (!otherParent) {
          parent_.store(nullptr);
          break;
        }
        if (other.parent_.compare_exchange_strong(otherParent, nullptr)) {
          parent_.store(otherParent);
          break;
        }
      }
      return *this;
    }
    T* operator->() {
      return get();
    }
    ~PtrHandle() {
      maybeClose();
    }
  };

 protected:
  T* getInstancePtr() {
    return &instance_;
  }
  void releaseLock() {
    mutex_.unlock();
  }
  friend class PtrHandle;

 public:

  PtrHandle getHandle() {
    return PtrHandle(this);
  }

  ~Synchronized() {
    mutex_.lock();
    mutex_.unlock();
  }

};

}} // kclpp::locks

