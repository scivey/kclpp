#include <glog/logging.h>
#include "kclpp/ScopeGuard.h"
#include "kclpp/locks/PThreadSpinLock.h"
#include "kclpp/posix/PosixError.h"

namespace kclpp { namespace locks {
using kclpp::posix::PosixError;

PThreadSpinLock::PThreadSpinLock(PThreadSpinLock&& other)
  : spinLock_(other.spinLock_) {
  other.spinLock_ = nullptr;
}

PThreadSpinLock::PThreadSpinLock(pthread_spinlock_t* spinLock)
  : spinLock_(spinLock) {
  DCHECK(!!spinLock_);
}

PThreadSpinLock& PThreadSpinLock::operator=(PThreadSpinLock&& other) {
  std::swap(spinLock_, other.spinLock_);
  return *this;
}

Outcome<Optional<PThreadSpinLock>, PosixError> PThreadSpinLock::create() {
  pthread_spinlock_t* spinner {nullptr};
  kclpp::ScopeGuard guard {[&spinner](){
    if (spinner) {
      delete spinner;
    }
  }};
  spinner = new pthread_spinlock_t;
  if (pthread_spin_init(spinner, PTHREAD_PROCESS_PRIVATE) != 0) {
    return Outcome<Optional<PThreadSpinLock>, PosixError> {
      PosixError::fromErrno(errno, "pthread_spin_init()")
    };
  }
  guard.dismiss();
  return Outcome<Optional<PThreadSpinLock>, PosixError> {
    Optional<PThreadSpinLock> {PThreadSpinLock{spinner}}
  };
}

bool PThreadSpinLock::good() const {
  return !!spinLock_;
}

PThreadSpinLock::operator bool() const {
  return good();
}

bool PThreadSpinLock::try_lock() {
  DCHECK(good());
  return pthread_spin_trylock(spinLock_) == 0;
}

void PThreadSpinLock::lock() {
  DCHECK(good());

  int rc = pthread_spin_lock(spinLock_);
  // this should only return non-zero in pathological cases.
  // since I don't know what those are and I don't have a clever way
  // in mind of handling them, it's best to just fail here with an assert.

  // normally I would give this kind of thing a Try<Unit> return value,
  // but this function signature is needed to implement the "Lockable"
  // c++11 concept (i.e. to maintain compatibility with std::lock_guard and friends)
  CHECK(rc == 0) << "pthread_spin_lock(): " << strerror(rc);
}

void PThreadSpinLock::unlock() {
  DCHECK(good());
  // see note above on `lock()` for reasoning behind a hard assert-fail here
  int rc = pthread_spin_unlock(spinLock_);
  CHECK(rc == 0) << "pthread_spin_unlock(): " << strerror(rc);
}

}} // kclpp::locks
