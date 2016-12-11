#pragma once

#include "kclpp/posix/PosixError.h"
#include "kclpp/Outcome.h"
#include "kclpp/Optional.h"

#include <pthread.h>

namespace kclpp { namespace locks {


class PThreadSpinLock {
 protected:
  pthread_spinlock_t *spinLock_ {nullptr};
  PThreadSpinLock(const PThreadSpinLock&) = delete;
  PThreadSpinLock& operator=(const PThreadSpinLock&) = delete;
  PThreadSpinLock();
  PThreadSpinLock(pthread_spinlock_t*);
 public:
  PThreadSpinLock(PThreadSpinLock&&);
  PThreadSpinLock& operator=(PThreadSpinLock&&);
  static Outcome<Optional<PThreadSpinLock>, posix::PosixError> create();
  bool good() const;
  explicit operator bool() const;
  bool try_lock();
  void lock();
  void unlock();
};


}} // kclpp::locks
