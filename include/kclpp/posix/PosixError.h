#pragma once
#include <string>
#include <cstring>

#include <sstream>
#include <errno.h>
#include <sys/types.h>

#include "kclpp/KCLPPError.h"

namespace kclpp { namespace posix {

class PosixError: public KCLPPError {
 public:
  PosixError(){}

  template<typename T>
  PosixError(const T& msg): KCLPPError(msg){}

  static PosixError fromErrno(int errn, const std::string& msg = "");
};


}} // kclpp::posix
