#pragma once
#include "kclpp/vendored/folly/Optional.h"

namespace kclpp {

template<typename T>
using Optional = kclpp::vendored::folly::Optional<T>;

} // kclpp
