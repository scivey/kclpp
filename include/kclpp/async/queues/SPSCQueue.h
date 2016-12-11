#pragma once

#include "kclpp/async/vendored/readerwriterqueue/readerwriterqueue.h"

namespace kclpp { namespace async { namespace queues {

template<typename T>
using SPSCQueue = kclpp::async::vendored::moodycamel::ReaderWriterQueue<T>;

}}} // kclpp::async::queues