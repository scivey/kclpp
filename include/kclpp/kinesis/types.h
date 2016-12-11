#pragma once
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/core/utils/memory/stl/AWSVector.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include "kclpp/Optional.h"
#include "kclpp/macros/proxies.h"

namespace kclpp { namespace kinesis {

KCLPP_DEFINE_STRING_PROXY_TYPE(StreamName);
KCLPP_DEFINE_STRING_PROXY_TYPE(ShardID);
KCLPP_DEFINE_STRING_PROXY_TYPE(ShardIteratorID);
KCLPP_DEFINE_UNSIGNED_PROXY_TYPE(NumberOfRecords);

}} // kclpp::kinesis

