#include "kclpp/clientlib/worker/ShardConsumer.h"
#include "kclpp/clientlib/worker/ShardConsumerIf.h"
#include "kclpp/clientlib/worker/ShardConsumerStateType.h"
#include "kclpp/clientlib/worker/ShardConsumerError.h"
#include "kclpp/clientlib/worker/ShardInfo.h"
#include "kclpp/clientlib/worker/StreamConfig.h"
#include "kclpp/clientlib/RecordProcessorCheckpointerIf.h"
#include "kclpp/clientlib/record_processor/InitializeInput.h"
#include "kclpp/clientlib/record_processor/RecordProcessor.h"
#include "kclpp/clientlib/ShutdownReason.h"
#include "kclpp/clientlib/ShutdownNotification.h"
#include "kclpp/clientlib/record_processor/ProcessRecordsInput.h"
#include "kclpp/leases/ExtendedSequenceNumber.h"
#include "kclpp/leases/LeaseCheckpointerIf.h"
#include "kclpp/kinesis/KCLKinesisClient.h"
#include "kclpp/kinesis/ShardIterator.h"
#include "kclpp/AsyncGroup.h"
#include "kclpp/Outcome.h"
#include "kclpp/Unit.h"
#include "kclpp/util/misc.h"

namespace kclpp { namespace clientlib { namespace worker {


template<typename TErr,
  typename = decltype(std::declval<TErr>().GetMessage())>
ShardConsumerError makeConsumerError(const TErr& awsError, const std::string& extra = "") {
  std::ostringstream oss;
  oss << awsError.GetExceptionName() << " : '" << awsError.GetMessage() << "' (" << extra << ")";
  return ShardConsumerError {oss.str()};
}


ShardConsumer::ShardConsumer(ShardConsumerParams&& params)
  : params_(std::forward<ShardConsumerParams>(params)) {}

ShardConsumer* ShardConsumer::createNew(ShardConsumerParams&& params) noexcept {
  DCHECK(!!params.leaseManager);
  DCHECK(!!params.asyncContext);
  DCHECK(!!params.recordProcessor);
  DCHECK(!!params.kinesisClient);
  DCHECK(!!params.checkpointer);
  DCHECK(!!params.processorCheckpointer);
  return new ShardConsumer(std::forward<ShardConsumerParams>(params));
}

void ShardConsumer::startInitializing(unit_cb_t callback) noexcept {
  auto initState = getInitialShardConsumerStateType();
  DCHECK((
    state_.stateType == initState
    || state_.stateType == ShardConsumerStateType::FAILED_TO_INITIALIZE
  ));
  DCHECK(!state_.shardIterator.hasValue());
  state_.stateType = ShardConsumerStateType::WAITING_ON_PARENT_SHARDS;
  waitOnParentShards([this, callback](const auto& outcome) {
    if (!outcome.IsSuccess()) {
      callback(outcome);
      return;
    }
    this->state_.stateType = ShardConsumerStateType::INITIALIZING;
    doInitialize([this, callback](const auto& outcome) {
      if (!outcome.IsSuccess()) {
        LOG(INFO) << "doInitialize failure: " << outcome.GetError().what();
        callback(outcome);
        return;
      }
      this->state_.stateType = ShardConsumerStateType::READY_FOR_PROCESSING;
      callback(unit_outcome_t{Unit{}});
    });
  });
}

void ShardConsumer::onProcessingStop(const process_batch_outcome_t &outcome) noexcept {
  DCHECK(state_.stateType != ShardConsumerStateType::PROCESSING);
  if (state_.stateType == ShardConsumerStateType::SHUTDOWN_REQUESTED) {
    state_.stateType = ShardConsumerStateType::SHUTDOWN_COMPLETE;
    if (state_.shutdownCallback.hasValue()) {
      if (outcome.IsSuccess()) {
        state_.shutdownCallback.value()(unit_outcome_t{Unit{}});
      } else {
        state_.shutdownCallback.value()(unit_outcome_t{outcome.GetError()});
      }
    } else {
      DCHECK(outcome.IsSuccess()) << outcome.GetError().what();
    }
  }
}

void ShardConsumer::startProcessing(unit_cb_t callback) noexcept {
  DCHECK(state_.stateType == ShardConsumerStateType::READY_FOR_PROCESSING);
  DCHECK(state_.shardIterator.hasValue());
  DCHECK(!state_.shardIterator.value()->isAtEnd());
  record_processor::InitializeInput initialInput;
  initialInput.shardID = params_.shardInfo.getState().shardID;
  params_.recordProcessor->initialize(std::move(initialInput),
    [this, callback](const auto& outcome) {
      if (!outcome.IsSuccess()) {
        state_.stateType = ShardConsumerStateType::FAILED_TO_INITIALIZE;
        callback(unit_outcome_t{ShardConsumerError{outcome.GetError().what()}});
        return;
      }
      this->state_.stateType = ShardConsumerStateType::PROCESSING;
      this->processBatches([this](const auto& processOutcome) {
        this->onProcessingStop(processOutcome);
      });
      callback(unit_outcome_t{Unit{}});
    }
  );
}


void ShardConsumer::requestShutdown(unit_cb_t callback) noexcept {
  DCHECK(!state_.shutdownCallback.hasValue());
  state_.shutdownCallback.assign(callback);
  state_.stateType = ShardConsumerStateType::SHUTDOWN_REQUESTED;
}


void ShardConsumer::processNextBatchInner(process_batch_cb_t callback) noexcept {
  DCHECK(state_.stateType == ShardConsumerStateType::PROCESSING);
  DCHECK(state_.shardIterator.hasValue());
  DCHECK(!state_.shardIterator.value()->isAtEnd());
  ShardIterator::GetNextBatchRequest request;
  request.limit = params_.streamConfig.getState().maxRecordsPerCall.value();
  state_.shardIterator.value()->getNextBatch(request,
    [this, callback](const auto& outcome) {
      if (!outcome.IsSuccess()) {
        callback(process_batch_outcome_t{ShardConsumerError{outcome.GetError().GetMessage()}});
        return;
      }
      const auto& records = outcome.GetResult().GetRecords();
      bool shouldProcess = !records.empty()
        || params_.streamConfig.getState().callProcessRecordsIfEmpty;
      if (shouldProcess) {
        size_t numRecords = records.size();
        record_processor::ProcessRecordsInput processInput;
        processInput.records = records;
        processInput.checkpointer = params_.processorCheckpointer;
        params_.recordProcessor->processRecords(
          std::move(processInput),
          [this, callback, numRecords](const auto& outcome) {
            if (!outcome.IsSuccess()) {
              callback(process_batch_outcome_t{ShardConsumerError{outcome.GetError().what()}});
              return;
            }
            ProcessBatchResult result;
            result.numProcessed = numRecords;
            callback(process_batch_outcome_t{std::move(result)});
          }
        );
        return;
      } else {
        ProcessBatchResult result;
        result.numProcessed = 0;
        callback(process_batch_outcome_t{std::move(result)});
      }
    }
  );
}

void ShardConsumer::processBatches(process_batch_cb_t callback) noexcept {
  if (state_.stateType != ShardConsumerStateType::PROCESSING) {
    ProcessBatchResult result;
    result.numProcessed = 0;
    callback(process_batch_outcome_t{std::move(result)});
    return;
  }
  DCHECK(state_.stateType == ShardConsumerStateType::PROCESSING);
  processNextBatchInner([this, callback](const auto& outcome) {
    if (!outcome.IsSuccess()) {
      callback(outcome);
      return;
    }
    if (this->state_.stateType != ShardConsumerStateType::PROCESSING) {
      callback(outcome);
      return;
    }
    params_.asyncContext->runInEventThread([this, callback]() {
      this->processBatches(callback);
    });
  });
}

void ShardConsumer::waitOnParentShards(unit_cb_t callback) noexcept {
  const auto& parentIDs = params_.shardInfo.getState().parentShardIDs;
  // TODO: handle parent shard sync.
  DCHECK(parentIDs.empty());
  callback(unit_outcome_t{Unit{}});
}

void ShardConsumer::doInitialize(unit_cb_t callback) noexcept {
  params_.checkpointer->getCheckpoint(params_.shardInfo.getState().shardID,
    [this, callback](const auto& outcome) {
      if (!outcome.IsSuccess()) {
        callback(unit_outcome_t {makeConsumerError(outcome.GetError())});
        return;
      }
      DCHECK(!state_.shardIterator);
      auto result = outcome.GetResult();
      const auto& shardState = params_.shardInfo.getState();
      auto request = GetShardIteratorRequest()
        .WithStreamName(shardState.streamName.value())
        .WithShardId(shardState.shardID.value());

      if (result.sequenceNumber.hasValue()) {
        request
          .WithStartingSequenceNumber(
            result.sequenceNumber.value().sequenceNumber().value()
          )
          .WithShardIteratorType(
            ShardIteratorType::AFTER_SEQUENCE_NUMBER
          );
      } else {
        const auto& configState = params_.streamConfig.getState();
        switch (configState.initialPosition) {
          case InitialPositionInStream::TRIM_HORIZON:
            request.SetShardIteratorType(ShardIteratorType::TRIM_HORIZON);
            break;
          case InitialPositionInStream::LATEST:
            request.SetShardIteratorType(ShardIteratorType::LATEST);
            break;
        }
      }
      params_.kinesisClient->getShardIteratorAsync(std::move(request),
        [this, callback](const auto& outcome) {
          if (!outcome.IsSuccess()) {
            callback(unit_outcome_t{
              makeConsumerError(outcome.GetError())
            });
            return;
          }
          auto shardIter = util::createUnique<kinesis::ShardIterator>(
            this->params_.kinesisClient, outcome.GetResult()
          );
          if (shardIter->isAtEnd()) {
            callback(unit_outcome_t{ShardConsumerError{"Already at end of shard."}});
            return;
          }
          this->state_.shardIterator = std::move(shardIter);
          callback(unit_outcome_t{Unit{}});
        }
      );
    }
  );
}

}}} // kclpp::clientlib::worker