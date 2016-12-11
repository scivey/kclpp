#pragma once
#include "kclpp/KCLPPError.h"
#include "kclpp/Outcome.h"

namespace kclpp { namespace posix {

class FileDescriptor {
 public:
  KCLPP_DECLARE_EXCEPTION(Invalid, KCLPPError);
 protected:
  int fd_ {0};
  FileDescriptor(const FileDescriptor&) = delete;
  FileDescriptor& operator=(const FileDescriptor&) = delete;
  explicit FileDescriptor(int fd);
 public:
  FileDescriptor();
  FileDescriptor(FileDescriptor&& other);
  bool good() const;
  operator bool() const;
  using get_outcome_t = Outcome<int, Invalid>;
  get_outcome_t get();
  get_outcome_t release();
  FileDescriptor& operator=(FileDescriptor&& other);
  void maybeClose();
  ~FileDescriptor();
  static FileDescriptor takeOwnership(int fd);
};

}} // kclpp::posix
