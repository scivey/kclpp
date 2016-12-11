#pragma once
#include "kclpp/async/EventError.h"
#include "kclpp/KCLPPError.h"

namespace kclpp { namespace async { namespace queues {

#define X KCLPP_DECLARE_EXCEPTION
X(QueueError, EventError);
X(QueueWriteError, QueueError);
X(PartialWriteError, QueueWriteError);
X(QueueFull, QueueWriteError);
#undef X

}}} // kclpp::async::queues