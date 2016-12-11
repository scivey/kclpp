#pragma once

namespace kclpp { namespace clientlib {

enum class SentinelCheckpoint {
  TRIM_HORIZON,
  LATEST,
  SHARD_END,
  AT_TIMESTAMP
};

}} // kclpp::clientlib
