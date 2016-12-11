#pragma once

#include "kclpp/async/vendored/concurrentqueue/blockingconcurrentqueue.h"

namespace kclpp { namespace async { namespace queues {

template<typename T>
using MPMCQueue = kclpp::async::vendored::moodycamel::BlockingConcurrentQueue<T>;

}}} // kclpp::async::queues