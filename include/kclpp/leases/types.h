#pragma once
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/core/utils/memory/stl/AWSVector.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include "kclpp/Optional.h"
#include "kclpp/macros/proxies.h"

namespace kclpp { namespace leases {

#ifdef X
  #undef X
#endif

#define KCLPP_LEASE_STRING_PROXY_TYPES \
  X(LeaseKey) \
  X(LeaseOwner) \
  X(ConcurrencyToken) \
  X(CheckpointID) \
  X(SubCheckpointID)


#define KCLPP_LEASE_UNSIGNED_PROXY_TYPES \
  X(LeaseCounter) \
  X(NumberOfLeaseSwitches) \
  X(NumberOfLeases)


#define X(arg) KCLPP_DEFINE_STRING_PROXY_TYPE(arg)
KCLPP_LEASE_STRING_PROXY_TYPES
#undef X
#define X(arg) KCLPP_DEFINE_UNSIGNED_PROXY_TYPE(arg)
KCLPP_LEASE_UNSIGNED_PROXY_TYPES
#undef X

}} // kclpp::leases

namespace std {
#define X(arg) KCLPP_DEFINE_PROXY_HASH(kclpp::leases, arg)
KCLPP_LEASE_STRING_PROXY_TYPES
KCLPP_LEASE_UNSIGNED_PROXY_TYPES
#undef X
} // std

#undef KCLPP_LEASE_STRING_PROXY_TYPES
#undef KCLPP_LEASE_UNSIGNED_PROXY_TYPES
