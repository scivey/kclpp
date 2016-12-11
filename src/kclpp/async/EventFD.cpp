#include "kclpp/async/EventFD.h"
#include <glog/logging.h>
#include <sys/eventfd.h>

using kclpp::posix::FileDescriptor;

namespace kclpp { namespace async {

EventFD::EventFD(FileDescriptor&& fd)
  : fd_(std::forward<FileDescriptor>(fd)){}

using create_outcome_t = EventFD::create_outcome_t;
create_outcome_t EventFD::create() {
  int fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
  if (fd <= 0) {
    return create_outcome_t {
      CouldntCreate{ strerror(errno) }
    };
  }
  return create_outcome_t {
    Optional<EventFD>{EventFD{FileDescriptor::takeOwnership(fd)}}
  };
}

using get_outcome_t = EventFD::get_outcome_t;
get_outcome_t EventFD::getFDNum() {
  return fd_.get();
}

static const std::string kEventFDNotReadyMsg {"NotReady"};

using read_outcome_t = EventFD::read_outcome_t;
read_outcome_t EventFD::read() {
  auto fdNum = getFDNum();
  if (!fdNum.IsSuccess()) {
    return read_outcome_t {
      ReadError {"Invalid file descriptor."}
    };
  }
  int fd = fdNum.GetResult();
  int64_t buff;
  errno = 0;
  ssize_t nr = ::read(fd, (void*) &buff, sizeof(buff));
  if (nr < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return read_outcome_t {
        NotReady {kEventFDNotReadyMsg}
      };
    } else {
      return read_outcome_t {
        ReadError{ strerror(errno) }
      };
    }
  } else if (nr == 0) {
    if (errno == 0) {
      return read_outcome_t {
        NotReady { kEventFDNotReadyMsg }
      };
    } else {
      return read_outcome_t {
        NotReady {strerror(errno)}
      };
    }
  }
  DCHECK(nr == 8);
  return read_outcome_t {buff};
}

using write_outcome_t = EventFD::write_outcome_t;
write_outcome_t EventFD::write(int64_t num) {
  auto fdNum = getFDNum();
  if (!fdNum.IsSuccess()) {
    return write_outcome_t {
      WriteError {"Invalid file descriptor."}
    };
  }
  int fd = fdNum.GetResult();
  int64_t buff = num;
  ssize_t nr = ::write(fd, (void*) &buff, sizeof(buff));
  if (nr < 0) {
    return write_outcome_t {
      WriteError {strerror(errno)}
    };
  } else if (nr != sizeof(buff)) {
    return write_outcome_t {
      WriteError {"Unknown write error."}
    };
  }
  return write_outcome_t{Unit{}};
}

bool EventFD::good() const {
  return !!fd_;
}

EventFD::operator bool() const {
  return good();
}

}} // kclpp::async

