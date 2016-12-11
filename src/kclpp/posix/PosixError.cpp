#include "kclpp/posix/PosixError.h"
#include <string>
#include <cstring>
#include <sstream>
#include <errno.h>
#include <sys/types.h>

namespace kclpp { namespace posix {

PosixError PosixError::fromErrno(int errn, const std::string& msg) {
  std::ostringstream oss;
  oss << "PosixError( [" << errn << "] '" << strerror(errn) << "' : '" << msg << "'";
  return PosixError(oss.str());
}


}} // kclpp::posix
