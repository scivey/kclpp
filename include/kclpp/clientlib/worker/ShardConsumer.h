#pragma once
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


namespace kclpp {

  namespace kinesis  {
    class KCLKinesisClient;
  } // kinesis

  namespace leases {
    class LeaseManagerIf;
    class LeaseCheckpointerIf;
  } // leases

  namespace async {
    class KCLAsyncContext;
  }

  namespace record_processor {
    class RecordProcessor;
  }

} // kclpp

namespace kclpp { namespace clientlib { namespace worker {

class ShardConsumer: public ShardConsumerIf,
                     public std::enable_shared_from_this<ShardConsumer> {
 public:

  using ShardIteratorType = Aws::Kinesis::Model::ShardIteratorType;
  using GetShardIteratorRequest = Aws::Kinesis::Model::GetShardIteratorRequest;
  using ShardIterator = kinesis::ShardIterator;

  using unit_outcome_t = Outcome<Unit, ShardConsumerError>;
  using unit_cb_t = std::function<void (const unit_outcome_t&)>;

  struct ShardConsumerOptions {
    std::chrono::milliseconds taskBackoffTime {0};
    std::chrono::milliseconds parentShardPollInterval {0};
    bool skipShardSyncAtWorkerInitializationIfLeasesExist {false};
    bool cleanupLeasesOfCompletedShards {true};
  };

  struct ShardConsumerParams {
    ShardConsumerOptions options;
    ShardInfo shardInfo;
    StreamConfig streamConfig;
    std::shared_ptr<kinesis::KCLKinesisClient> kinesisClient {nullptr};
    std::shared_ptr<leases::LeaseManager> leaseManager {nullptr};
    std::shared_ptr<record_processor::RecordProcessor> recordProcessor {nullptr};
    std::shared_ptr<leases::LeaseCheckpointerIf> checkpointer {nullptr};
    std::shared_ptr<RecordProcessorCheckpointerIf> processorCheckpointer {nullptr};
    std::shared_ptr<async::KCLAsyncContext> asyncContext {nullptr};
  };

  struct ShardConsumerState {
    Optional<std::unique_ptr<ShutdownNotification>> shutdownNotification {nullptr};
    Optional<leases::ExtendedSequenceNumber> checkpoint {nullptr};
    Optional<ShutdownReason> shutdownReason;
    Optional<std::unique_ptr<ShardIterator>> shardIterator;
    ShardConsumerStateType stateType {getInitialShardConsumerStateType()};
    Optional<unit_cb_t> shutdownCallback;
    bool hasPendingProcessCallback {false};
  };

  struct ProcessBatchResult {
    size_t numProcessed {0};
  };
  using process_batch_outcome_t = Outcome<ProcessBatchResult, ShardConsumerError>;
  using process_batch_cb_t = std::function<void (const process_batch_outcome_t&)>;

 protected:
  ShardConsumerParams params_;
  ShardConsumerState state_;
  ShardConsumer(ShardConsumerParams&& params);
 public:
  static ShardConsumer* createNew(ShardConsumerParams&& params) noexcept;
  void startInitializing(unit_cb_t callback) noexcept;
 protected:
  void onProcessingStop(const process_batch_outcome_t &outcome) noexcept;

 public:
  void startProcessing(unit_cb_t callback) noexcept;
  void requestShutdown(unit_cb_t callback) noexcept;

 protected:

  void processNextBatchInner(process_batch_cb_t callback) noexcept;
  void processBatches(process_batch_cb_t callback) noexcept;
  void waitOnParentShards(unit_cb_t callback) noexcept;
  void doInitialize(unit_cb_t callback) noexcept;
};

}}} // kclpp::clientlib::worker