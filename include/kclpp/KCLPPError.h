#pragma once

#include <stdexcept>

namespace kclpp {

class KCLPPError: public std::runtime_error {
 public:
  KCLPPError(): std::runtime_error("KCLPPError") {}

  template<typename T>
  KCLPPError(const T& msg): std::runtime_error(msg) {}
};

#define KCLPP_DECLARE_EXCEPTION(class_name, parent_class) \
  class class_name: public parent_class { \
   public: \
    template<typename T> \
    class_name(const T& msg): parent_class(msg){} \
    \
    class_name() {} \
  };


KCLPP_DECLARE_EXCEPTION(InvalidState, KCLPPError);
KCLPP_DECLARE_EXCEPTION(DeserializationError, KCLPPError);
} // kclpp
