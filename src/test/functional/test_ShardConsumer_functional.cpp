#include <gtest/gtest.h>
#include <utility>
#include <type_traits>

#include <aws/kinesis/model/DeleteStreamRequest.h>
#include <aws/kinesis/model/CreateStreamRequest.h>
#include <aws/kinesis/model/GetRecordsRequest.h>

#include <aws/kinesis/model/StreamStatus.h>
#include <aws/kinesis/model/PutRecordsRequest.h>
#include <aws/kinesis/model/ShardIteratorType.h>
#include <aws/kinesis/model/GetShardIteratorRequest.h>
#include <aws/kinesis/model/GetShardIteratorResult.h>
#include <aws/kinesis/model/PutRecordsResult.h>
#include <aws/kinesis/model/PutRecordsRequestEntry.h>
#include <aws/kinesis/model/Shard.h>
#include <aws/kinesis/model/Record.h>

#include <aws/kinesis/KinesisErrors.h>

#include "kclpp/clientlib/worker/ShardConsumer.h"
#include "kclpp/clientlib/worker/ShardInfo.h"
#include "kclpp/clientlib/worker/StreamConfig.h"
#include "kclpp/leases/ExtendedSequenceNumber.h"
#include "kclpp/leases/LeaseManager.h"
#include "kclpp/leases/LeaseCheckpointer.h"
#include "kclpp/leases/LeaseCheckpointerIf.h"

#include "kclpp/kinesis/KCLKinesisClient.h"
#include "kclpp/kinesis/types.h"
#include "kclpp/Unit.h"

#include "kclpp/async/KCLAsyncContext.h"
#include "kclpp/dynamo/KCLDynamoClient.h"
#include "kclpp_test_support/LeaseManagerTestContext.h"
#include "kclpp_test_support/LeaseCoordinatorTestContext.h"
#include "kclpp_test_support/KinesisClientTestContext.h"
#include "kclpp_test_support/KinesisRecordCRUDTestContext.h"
#include "kclpp_test_support/TableGuard.h"
#include "kclpp_test_support/MockClock.h"
#include "kclpp_test_support/testConfig.h"
#include "kclpp_test_support/misc.h"

using Aws::Client::ClientConfiguration;
using Aws::Kinesis::Model::CreateStreamRequest;
using Aws::Kinesis::Model::DeleteStreamRequest;

using namespace std;
using kclpp::locks::ThreadBaton;
using kclpp::clientlib::worker::ShardConsumer;


using kclpp_test_support::LeaseManagerTestContext;
using kclpp_test_support::makePostGuard;
using kclpp_test_support::TableGuard;
using kclpp_test_support::MockClock;
using kclpp_test_support::buildSimpleLease;
using kclpp_test_support::buildSimpleLeaseState;
using kclpp_test_support::makeClientConfig;
using kclpp_test_support::KinesisClientTestContext;
using kclpp_test_support::LeaseCoordinatorTestContext;
using kclpp_test_support::KinesisRecordCRUDTestContext;

using kclpp_test_support::loopOnBaton;


using kclpp::Unit;
using namespace kclpp::clientlib;
using namespace kclpp::clientlib::worker;
using namespace kclpp::clientlib::record_processor;
using namespace kclpp::clock;
using namespace kclpp::leases;


namespace util = kclpp::util;
using kclpp::async::KCLAsyncContext;
using kclpp::dynamo::KCLDynamoClient;
using kclpp::dynamo::TableName;
using kclpp::kinesis::KCLKinesisClient;
using kclpp::kinesis::StreamName;
using kclpp::kinesis::NumberOfRecords;
using kclpp::kinesis::ShardID;

using kclpp::leases::LeaseManager;
using kclpp::leases::ConcurrencyToken;
using kclpp::leases::LeaseManagerIf;
using kclpp::leases::ExtendedSequenceNumber;
using kclpp::leases::LeaseCheckpointer;
using kclpp::leases::LeaseCheckpointerIf;
using kclpp::leases::CheckpointID;
using kclpp::leases::SubCheckpointID;


using ShardConsumerParams = ShardConsumer::ShardConsumerParams;
using ShardConsumerOptions = ShardConsumer::ShardConsumerOptions;
using ShardInfoState = ShardInfo::ShardInfoState;
using LeaseManagerParams = LeaseManager::LeaseManagerParams;

class SomeRecordProcessor: public RecordProcessor {
 public:
  virtual void initialize(InitializeInput&& arg, unit_cb_t callback) override {
    callback(unit_outcome_t{Unit{}});
  }
  virtual void processRecords(ProcessRecordsInput&& arg, unit_cb_t callback) override {
    callback(unit_outcome_t{Unit{}});
  }
  virtual void shutdown(ShutdownInput&& arg, unit_cb_t callback) override {
    callback(unit_outcome_t{Unit{}});
  }
};

class MockRecordProcessorCheckpointer: public RecordProcessorCheckpointerIf {
 protected:
  std::vector<ExtendedSequenceNumber> seqNums_;
 public:
  void checkpoint(const ExtendedSequenceNumber& seqNum) {
    seqNums_.push_back(seqNum);
  }
  const decltype(seqNums_)& getSeqNums() const {
    return seqNums_;
  }
};


TEST(TestShardConsumer, TestSanity1) {
  auto managerContext = std::make_shared<LeaseManagerTestContext>();
  auto& asyncContext = managerContext->asyncContext;
  const string kTableName = managerContext->tableName;
  const size_t kNumShards {4};
  const string kWorkerID {"this-worker-id"};
  {
    auto kinesisContext = std::make_shared<KinesisClientTestContext>(asyncContext);
    auto kinesisClient = kinesisContext->client;
    LeaseCoordinatorTestContext coordinatorContext {managerContext};
    {
      auto leaseCheckpointer = std::make_shared<LeaseCheckpointer>(
        coordinatorContext.coordinator
      );
      KinesisRecordCRUDTestContext recordCrudContext {kinesisContext}; 
      const string kStreamName = recordCrudContext.kStreamName;
      auto tableGuard = TableGuard::create(managerContext.get());
      {
        auto setTimeAndSave = [managerContext](Lease::LeaseState&& leaseState) {
          leaseState.lastCounterIncrementNanoseconds.assign(
            NanosecondPoint{ MockClock::secToNanosec(1000) }
          );
          auto lease = std::make_shared<Lease>(std::move(leaseState));
          managerContext->saveLeaseSync(lease);
        };
        for (size_t i = 0; i < recordCrudContext.shards.size(); i++) {
          Lease::LeaseState leaseState;
          auto& shardData = recordCrudContext.shards.at(i);
          leaseState.leaseKey = LeaseKey {shardData.GetShardId()};
          if (i < 3) {
            leaseState.leaseOwner = LeaseOwner {kWorkerID};
          } else {
            leaseState.leaseOwner = LeaseOwner {"other-worker-id"};
          }
          leaseState.lastCounterIncrementNanoseconds.assign(
            NanosecondPoint { MockClock::secToNanosec(1000) }
          );
          auto lease = std::make_shared<Lease>(std::move(leaseState));
          managerContext->saveLeaseSync(lease);
        }
      }
      {
        ShardConsumerParams params;
        params.asyncContext = asyncContext;
        params.kinesisClient = kinesisClient;
        params.leaseManager = managerContext->manager;
        params.checkpointer = std::shared_ptr<LeaseCheckpointerIf>{
          leaseCheckpointer.get(), [](LeaseCheckpointerIf *ptr) {}
        };
        params.processorCheckpointer = std::make_shared<MockRecordProcessorCheckpointer>();
        {
          ShardConsumerOptions options;
          options.taskBackoffTime = std::chrono::milliseconds {50};
          options.parentShardPollInterval = std::chrono::milliseconds {50};
          params.options = options;
          ShardInfoState infoState;
          infoState.streamName = StreamName {kStreamName};
          infoState.shardID = ShardID {recordCrudContext.firstShardID};
          infoState.concurrencyToken = ConcurrencyToken {"ctoken"};
          infoState.currentCheckpoint = ExtendedSequenceNumber {
            CheckpointID {"000"},
            SubCheckpointID {"000"}
          };
          params.shardInfo = ShardInfo(std::move(infoState));
          StreamConfig::StreamConfigState configState;
          configState.maxRecordsPerCall = NumberOfRecords {10};
          configState.streamName = StreamName {kStreamName};
          params.streamConfig = StreamConfig{std::move(configState)};
        }
        {
          params.leaseManager = managerContext->manager;
          params.recordProcessor = std::shared_ptr<SomeRecordProcessor>{
            new SomeRecordProcessor
          };
        }
        {
          ThreadBaton bat;
          coordinatorContext.coordinator->start([&bat](const auto& outcome) {
            auto guard = makePostGuard(bat);
            EXPECT_TRUE(outcome.IsSuccess())
              << outcome.GetError().GetMessage();
          });
          managerContext->loopOnBaton(bat);
        }
        {
          auto consumer = util::createShared<ShardConsumer>(std::move(params));
          {
            ThreadBaton bat;
            consumer->startInitializing([&bat](const auto& outcome) {
              auto guard = makePostGuard(bat);
              EXPECT_TRUE(outcome.IsSuccess())
                << outcome.GetError().what();
            });
            managerContext->loopOnBaton(bat);
          }
          {
            ThreadBaton bat;
            consumer->startProcessing([&bat](const auto& outcome) {
              auto guard =makePostGuard(bat);
              EXPECT_TRUE(outcome.IsSuccess());
            });
            managerContext->loopOnBaton(bat);
          }
          {
            auto startTime = std::chrono::system_clock::now();
            for (;;) {
              auto nowTime = std::chrono::system_clock::now();
              std::chrono::nanoseconds delta = nowTime - startTime;
              if (delta.count() > 100000000) {
                break;
              }
              managerContext->asyncContext->getEventContext()->getBase()->runOnce();
              this_thread::sleep_for(chrono::milliseconds(10));
            }
          }
          {
            ThreadBaton bat;
            consumer->requestShutdown([&bat](const auto& outcome) {
              auto guard = makePostGuard(bat);
              EXPECT_TRUE(outcome.IsSuccess());
            });
            managerContext->loopOnBaton(bat);
          }
        }
        {
          ThreadBaton bat;
          coordinatorContext.coordinator->stop([&bat](const auto& outcome) {
            auto guard = makePostGuard(bat);
            EXPECT_TRUE(outcome.IsSuccess())
              << outcome.GetError().GetMessage();
          });
          managerContext->loopOnBaton(bat);
        }
      }
    }
  }
}
