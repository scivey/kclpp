#pragma once
#include <memory>
#include <utility>

namespace kclpp {

// "adapted" (mostly stolen) from Folly.
template <class T>
class MoveWrapper {
 public:
  /** If value can be default-constructed, why not?
      Then we don't have to move it in */
  MoveWrapper() = default;

  /// Move a value in.
  explicit
  MoveWrapper(T&& t) : value(std::move(t)) {}

  /// copy is move
  MoveWrapper(const MoveWrapper& other) : value(std::move(other.value)) {}

  /// move is also move
  MoveWrapper(MoveWrapper&& other) : value(std::move(other.value)) {}

  const T& operator*() const { return value; }
        T& operator*()       { return value; }

  const T* operator->() const { return &value; }
        T* operator->()       { return &value; }

  /// move the value out (sugar for std::move(*moveWrapper))
  T&& move() { return std::move(value); }

  // SI : addition for friendlier `auto x = moveWrapperInstance.something()`
  T unwrap() {
    return move();
  }
  // If you want these you're probably doing it wrong, though they'd be
  // easy enough to implement
  MoveWrapper& operator=(MoveWrapper const&) = delete;
  MoveWrapper& operator=(MoveWrapper&&) = delete;

 private:
  mutable T value;
};

/// Make a MoveWrapper from the argument. Because the name "makeMoveWrapper"
/// is already quite transparent in its intent, this will work for lvalues as
/// if you had wrapped them in std::move.
template <class T, class T0 = typename std::remove_reference<T>::type>
MoveWrapper<T0> makeMoveWrapper(T&& t) {
  return MoveWrapper<T0>(std::forward<T0>(t));
}

} // kclpp
