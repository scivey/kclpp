#pragma once

#include "kclpp/posix/FileDescriptor.h"
#include "kclpp/KCLPPError.h"
#include "kclpp/posix/PosixError.h"
#include "kclpp/Unit.h"
#include "kclpp/Outcome.h"
#include "kclpp/Optional.h"

#include <utility>
#include <type_traits>

namespace kclpp { namespace async {

class EventFD {
 protected:
  posix::FileDescriptor fd_;
  EventFD(posix::FileDescriptor&&);
 public:
  KCLPP_DECLARE_EXCEPTION(EventFDError, posix::PosixError);
  KCLPP_DECLARE_EXCEPTION(CouldntCreate, EventFDError);
  KCLPP_DECLARE_EXCEPTION(Invalid, EventFDError);
  KCLPP_DECLARE_EXCEPTION(IOError, EventFDError);
  KCLPP_DECLARE_EXCEPTION(ReadError, IOError);
  KCLPP_DECLARE_EXCEPTION(NotReady, ReadError);
  KCLPP_DECLARE_EXCEPTION(WriteError, IOError);

  using create_outcome_t = Outcome<Optional<EventFD>, CouldntCreate>;
  static create_outcome_t create();

  using get_outcome_t = Outcome<int, posix::FileDescriptor::Invalid>;
  get_outcome_t getFDNum();

  using read_outcome_t = Outcome<int64_t, EventFDError>;
  read_outcome_t read();

  using write_outcome_t = Outcome<Unit, EventFDError>;
  write_outcome_t write(int64_t num);
  bool good() const;
  explicit operator bool() const;
};


}} // evs::events

