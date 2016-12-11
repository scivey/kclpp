#include "kclpp/posix/FileDescriptor.h"
#include <glog/logging.h>

namespace kclpp { namespace posix {

FileDescriptor::FileDescriptor(int fd): fd_(fd){}

FileDescriptor::FileDescriptor(){}

FileDescriptor::FileDescriptor(FileDescriptor&& other): fd_(other.fd_) {
  other.fd_ = 0;
}

bool FileDescriptor::good() const {
  return fd_ > 0;
}

FileDescriptor::operator bool() const {
  return good();
}

using get_outcome_t = FileDescriptor::get_outcome_t;
get_outcome_t FileDescriptor::get() {
  if (good()) {
    return get_outcome_t {fd_};
  }
  return get_outcome_t{FileDescriptor::Invalid{ "Invalid file descriptor." }};
}

get_outcome_t FileDescriptor::release() {
  auto result = get();
  if (good()) {
    fd_ = 0;
  }
  return result;
}


FileDescriptor& FileDescriptor::operator=(FileDescriptor&& other) {
  std::swap(fd_, other.fd_);
  return *this;
}

void FileDescriptor::maybeClose() {
  if (fd_ > 0) {
    int rc = close(fd_);
    if (rc < 0) {
      LOG(INFO) << "error on close? : '" << strerror(errno) << "'";
    }
    fd_ = 0;
  }
}

FileDescriptor::~FileDescriptor() {
  maybeClose();
}

FileDescriptor FileDescriptor::takeOwnership(int fd) {
  return FileDescriptor(fd);
}

}} // kclpp::posix
