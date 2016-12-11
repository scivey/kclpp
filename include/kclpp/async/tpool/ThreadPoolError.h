#pragma once
#include "kclpp/KCLPPError.h"
#include "kclpp/async/EventError.h"

namespace kclpp { namespace async { namespace tpool {

KCLPP_DECLARE_EXCEPTION(ThreadPoolError, EventError);
KCLPP_DECLARE_EXCEPTION(NotRunning, ThreadPoolError);
KCLPP_DECLARE_EXCEPTION(InvalidSettings, ThreadPoolError);
KCLPP_DECLARE_EXCEPTION(AlreadyRunning, ThreadPoolError);

KCLPP_DECLARE_EXCEPTION(TaskError, ThreadPoolError);
KCLPP_DECLARE_EXCEPTION(InvalidTask, TaskError);
KCLPP_DECLARE_EXCEPTION(TaskThrewException, TaskError);
KCLPP_DECLARE_EXCEPTION(CouldntSendResult, TaskError);

}}} // kclpp::async::tpool
