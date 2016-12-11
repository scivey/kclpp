#pragma once

namespace kclpp { namespace clientlib {

enum class ShutdownReason {
  REQUESTED,
  TERMINATE,
  ZOMBIE
};

}} // kclpp::clientlib

