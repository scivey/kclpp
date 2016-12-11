#pragma once

#include "kclpp/async/KCLAsyncContext.h"
#include "kclpp/leases/LeaseCoordinator.h"
#include "kclpp/clock/Clock.h"
#include "kclpp/clientlib/record_processor/RecordProcessorFactory.h"

namespace kclpp { namespace clientlib { namespace worker {


struct WorkerOptions {
  std::string applicationName;
  using msec = std::chrono::milliseconds;
  msec idleInterval {1000};
  msec parentShardPollInterval {1000};
  msec taskBackoffTime {1000};
  msec failoverTime {1000};
  bool cleanupLeasesUponShardCompletion {false};
  bool skipShardSyncAtWorkerInitializationIfLeasesExist {false};
};

struct WorkerParams {
  WorkerOptions options;
  std::shared_ptr<clock::Clock> clock {nullptr};
  std::shared_ptr<async::KCLAsyncContext> asyncContext {nullptr};
  std::shared_ptr<record_processor::RecordProcessorFactory> recordProcessorFactory {nullptr};
  std::shared_ptr<leases::LeaseCoordinator> leaseCoordinator {nullptr};
};


}}} // kclpp::clientlib::worker