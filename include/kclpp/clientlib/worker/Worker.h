#pragma once

#include "kclpp/async/KCLAsyncContext.h"
#include "kclpp/leases/LeaseCoordinator.h"
#include "kclpp/clientlib/record_processor/RecordProcessorFactory.h"
#include "kclpp/clientlib/worker/WorkerState.h"
#include "kclpp/clientlib/worker/WorkerParams.h"
#include "kclpp/clientlib/worker/WorkerError.h"
#include "kclpp/ScopeGuard.h"

namespace kclpp { namespace clientlib { namespace worker {


class Worker {
 protected:
  std::unique_ptr<WorkerParams> params_ {nullptr};
  std::unique_ptr<WorkerState> state_ {nullptr};
  Worker(){}
 public:
  static Worker* createNew(std::unique_ptr<WorkerParams>&& params) {
    Worker *workerPtr {nullptr};
    auto guard = makeGuard([&workerPtr]() {
      if (workerPtr) {
        delete workerPtr;
        workerPtr = nullptr;
      }
    });
    workerPtr = new Worker;
    workerPtr->params_ = std::move(params);
    workerPtr->state_ = util::makeUnique<WorkerState>();
    guard.dismiss();
    return workerPtr;
  }
  static Worker* createNew(WorkerParams&& params) {
    return createNew(std::make_unique<WorkerParams>(std::move(params)));
  }

  using unit_outcome_t = Outcome<Unit, WorkerError>;
  using unit_cb_t = func::Function<void, unit_outcome_t>;

  void initialize(unit_cb_t callback) {
    DCHECK(!!params_ && !!state_);
    params_->leaseCoordinator->start([this, callback](const auto& initOutcome) {
      if (!initOutcome.IsSuccess()) {
        callback(unit_outcome_t{WorkerError{initOutcome.GetError().GetMessage()}});
        return;
      }
      callback(unit_outcome_t{Unit{}});
    });
  }

  WorkerParams* dangerouslyGetParams() {
    DCHECK(!!params_);
    return params_.get();
  }

  WorkerState* dangerouslyGetState() {
    DCHECK(!!state_);
    return state_.get();
  }
 protected:
  void processUntilStopped(unit_cb_t callback) {
    runProcessLoop([this, callback](const auto& processOutcome) {
      if (!processOutcome.IsSuccess()) {
        callback(processOutcome);
        return;
      }
      if (state_->shutdownState.isShutdownStarted()) {
        callback(unit_outcome_t{Unit{}});
        return;
      }
      processUntilStopped(callback);
    });
  }
 public:

  void runProcessLoop(unit_cb_t callback) {
    callback(unit_outcome_t{Unit{}});
  }

  void run(unit_cb_t callback) {
    if (state_->shutdownState.isShutdownStarted()) {
      callback(unit_outcome_t{WorkerError{"worker is shutting down"}});
      return;
    }
    initialize([this, callback](const auto& initOutcome) {
      if (!initOutcome.IsSuccess()) {
        callback(initOutcome);
        return;
      }
      processUntilStopped([this, callback](const auto& processOutcome) {
        this->finalShutdown();
        callback(processOutcome);
      });
    });
  }


  struct ShutdownResult {
    bool startedShutdown {false};
  };
  using shutdown_outcome_t = Outcome<ShutdownResult, WorkerError>;
  using shutdown_cb_t = func::Function<void, const shutdown_outcome_t&>;
  void shutdown(shutdown_cb_t callback) {
    if (!state_->shutdownState.setShutdown()) {
      LOG(INFO) << "already shutting down!";
      ShutdownResult result;
      result.startedShutdown = false;
      callback(shutdown_outcome_t{result});
      return;
    }

    DCHECK(state_->shutdownState.setStartTime(params_->clock->getNowMsec()));
    params_->leaseCoordinator->stop([this, callback](const auto& outcome) {
      if (!outcome.IsSuccess()) {
        callback(shutdown_outcome_t{WorkerError{outcome.GetError().GetMessage()}});
        return;
      }
      ShutdownResult result;
      result.startedShutdown = true;
      callback(shutdown_outcome_t{result});
    });
  }

  void finalShutdown() {
    DCHECK(state_->shutdownState.setShutdownComplete());
  }

};



}}} // kclpp::clientlib::worker
