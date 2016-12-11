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

#include "kclpp/async/KCLAsyncContext.h"
#include "kclpp/kinesis/KCLKinesisClient.h"
#include "kclpp/kinesis/ShardIterator.h"
#include "kclpp/APIGuard.h"
#include "kclpp/ScopeGuard.h"
#include "kclpp/UniqueToken.h"
#include "kclpp/clock/SystemClock.h"
#include "kclpp/locks/ThreadBaton.h"
#include "kclpp_test_support/misc.h"
#include "kclpp_test_support/testConfig.h"
#include "kclpp_test_support/TableGuard.h"
#include "kclpp_test_support/MockClock.h"
#include "kclpp_test_support/KinesisClientTestContext.h"
#include "kclpp_test_support/KinesisRecordCRUDTestContext.h"
#include "kclpp_test_support/StreamGuard.h"
#include "kclpp_test_support/misc_kinesis.h"
#include "kclpp_test_support/RecordInput.h"

using namespace std;
using namespace kclpp::dynamo;
using namespace kclpp::kinesis;
using namespace kclpp;
using kclpp::async::KCLAsyncContext;
using kclpp::locks::ThreadBaton;
using kclpp::clock::NanosecondPoint;
using kclpp::clock::NanosecondDuration;
using kclpp::clock::Clock;
using kclpp::clock::SystemClock;

using Aws::Client::ClientConfiguration;
using Aws::Kinesis::Model::CreateStreamRequest;
using Aws::Kinesis::Model::DeleteStreamRequest;
using Aws::Kinesis::Model::GetRecordsRequest;
using Aws::Kinesis::Model::PutRecordsRequest;
using Aws::Kinesis::Model::PutRecordsRequestEntry;
using Aws::Kinesis::Model::StreamStatus;
using Aws::Kinesis::Model::GetShardIteratorRequest;
using Aws::Kinesis::Model::ShardIteratorType;
using Aws::Kinesis::Model::Record;

using Aws::Kinesis::Model::Shard;

using Aws::Kinesis::KinesisErrors;

using kclpp_test_support::LeaseManagerTestContext;
using kclpp_test_support::makePostGuard;
using kclpp_test_support::TableGuard;
using kclpp_test_support::MockClock;
using kclpp_test_support::buildSimpleLease;
using kclpp_test_support::buildSimpleLeaseState;
using kclpp_test_support::makeClientConfig;
using kclpp_test_support::KinesisClientTestContext;
using kclpp_test_support::StreamGuard;
using kclpp_test_support::StreamGuardOptions;
using kclpp_test_support::getRecordData;
using kclpp_test_support::KinesisRecordCRUDTestContext;
using kclpp_test_support::RecordInput;


TEST(TestKCLKinesisClient, TestStreamCRUD) {
  KinesisClientTestContext context;
  const string kStreamName {"some-stream"};
  const size_t kNumShards {10};
  {
    ThreadBaton bat;
    context.client->listStreamsAsync([&bat](const auto& outcome) {
      auto guard = makePostGuard(bat);
      EXPECT_TRUE(outcome.IsSuccess())
        << outcome.GetError().GetMessage();
      EXPECT_EQ(0, outcome.GetResult().GetStreamNames().size());
    });
    context.loopOnBaton(bat);
  }
  {
    ThreadBaton bat;
    auto request = CreateStreamRequest()
      .WithStreamName(kStreamName)
      .WithShardCount(kNumShards);
    context.client->createStreamAsync(request, [&bat](const auto& outcome) {
      auto guard = makePostGuard(bat);
      EXPECT_TRUE(outcome.IsSuccess());
    });
    context.loopOnBaton(bat);
  }
  {
    ThreadBaton bat;
    context.client->listStreamsAsync([&bat, kStreamName](const auto& outcome) {
      auto guard = makePostGuard(bat);
      EXPECT_TRUE(outcome.IsSuccess())
        << outcome.GetError().GetMessage();
      auto names = outcome.GetResult().GetStreamNames();
      EXPECT_EQ(1, names.size());
      auto name = names.at(0);
      EXPECT_EQ(kStreamName, name);
    });
    context.loopOnBaton(bat); 
  }
  context.waitUntilStreamIsActive(kStreamName);
  {
    ThreadBaton bat;
    context.client->describeStreamAsync(kStreamName,
      [&bat, kStreamName, kNumShards](const auto& outcome) {
        auto guard = makePostGuard(bat);
        EXPECT_TRUE(outcome.IsSuccess());
        auto desc = outcome.GetResult().GetStreamDescription();
        EXPECT_EQ(kStreamName, desc.GetStreamName());
        EXPECT_EQ(StreamStatus::ACTIVE, desc.GetStreamStatus());
        auto shards = desc.GetShards();
        EXPECT_EQ(kNumShards, shards.size());
      }
    );
    context.loopOnBaton(bat);
  }
  {
    ThreadBaton bat;
    auto request = DeleteStreamRequest()
      .WithStreamName(kStreamName);
    context.client->deleteStreamAsync(request, [&bat](const auto& outcome) {
      auto guard = makePostGuard(bat);
      EXPECT_TRUE(outcome.IsSuccess());
    });
    context.loopOnBaton(bat);
  }
  context.waitUntilStreamDoesNotExist(kStreamName);
}


TEST(TestKCLKinesisClient, TestStreamGuardSanity) {
  KinesisClientTestContext context;
  const string kStreamName {"some-stream"};
  const size_t kNumShards {10};
  {
    auto streamGuard = StreamGuard::create(&context,
      StreamGuardOptions().withName(kStreamName).withNumShards(kNumShards)
    );
    {
      ThreadBaton bat;
      context.client->describeStreamAsync(kStreamName,
        [&bat](const auto& outcome) {
          auto guard = makePostGuard(bat);
          EXPECT_TRUE(outcome.IsSuccess());
        }
      );
      context.loopOnBaton(bat);
    }
  }
}





TEST(TestKCLKinesisClient, TestRecordCRUD1) {
  KinesisClientTestContext context;
  const string kStreamName {"some-stream"};
  {
    auto streamGuard = StreamGuard::create(&context,
      StreamGuardOptions().withName(kStreamName)
    );
    string firstShardID;
    {
      ThreadBaton bat;
      context.client->listShardsAsync(StreamName{kStreamName},
        [&bat, &firstShardID](const auto& outcome) {
          auto guard = makePostGuard(bat);
          EXPECT_TRUE(outcome.IsSuccess());
          auto shards = outcome.GetResult().shards;
          EXPECT_TRUE(shards.size() > 0);
          firstShardID = shards.at(0).GetShardId();
        }
      );
      context.loopOnBaton(bat);
    }
    {
      ThreadBaton bat;
      PutRecordsRequest request;
      request.SetStreamName(kStreamName);
      auto record = RecordInput("fish", "test1").toRequestEntry();
      record.SetExplicitHashKey("0");
      request.AddRecords(record);
      context.client->putRecordsAsync(request,
        [&bat](const auto& outcome) {
          auto guard = makePostGuard(bat);
          EXPECT_TRUE(outcome.IsSuccess())
            << "putRecordsAsync error: "
            << outcome.GetError().GetExceptionName()
            << " : '" << outcome.GetError().GetMessage() << "'";
        }
      );
      context.loopOnBaton(bat);
    }
    ShardIteratorDescription firstShardIterDesc;
    {
      ThreadBaton bat;
      auto request = GetShardIteratorRequest()
        .WithStreamName(kStreamName)
        .WithShardId(firstShardID)
        .WithShardIteratorType(ShardIteratorType::TRIM_HORIZON);
      context.client->getShardIteratorAsync(request,
        [&bat, &firstShardIterDesc](const auto& outcome) {
          auto guard = makePostGuard(bat);
          EXPECT_TRUE(outcome.IsSuccess());
          firstShardIterDesc = outcome.GetResult();
        }
      );
      context.loopOnBaton(bat);
    }
    const auto& props = firstShardIterDesc.getProperties();
    EXPECT_EQ(kStreamName, props.streamName.value());
    EXPECT_EQ(firstShardID, props.shardID.value());
    EXPECT_EQ(ShardIteratorType::TRIM_HORIZON, props.iteratorType);
    {
      ThreadBaton bat;
      context.client->getRecordsAsync(
        firstShardIterDesc.getIteratorID(),
        10,
        [&bat](const auto& outcome) {
          auto guard = makePostGuard(bat);
          EXPECT_TRUE(outcome.IsSuccess());
          auto records = outcome.GetResult().GetRecords();
          EXPECT_EQ(1, records.size());
          auto record = records.at(0);
          EXPECT_EQ("test1", getRecordData(record));
        }
      );

      context.loopOnBaton(bat);
    }
  }
}






TEST(TestKCLKinesisClient, TestRecordCRUD2) {
  KinesisRecordCRUDTestContext context;
  {
    EXPECT_EQ(10, context.shards.size());
    auto firstHashKey = context.getStartingHashKey(0);
    auto otherHashKey = context.getStartingHashKey(2);
    context.putRecords({
      {firstHashKey, "fish", "good1"},
      {otherHashKey, "nope", "bad1"},
      {firstHashKey, "salmon", "good2"},
      {otherHashKey, "yolo", "bad2"}
    });
    ShardIteratorDescription firstShardIterDesc;
    {
      ThreadBaton bat;
      auto request = GetShardIteratorRequest()
        .WithStreamName(context.kStreamName)
        .WithShardId(context.firstShardID)
        .WithShardIteratorType(ShardIteratorType::TRIM_HORIZON);
      context.getClient()->getShardIteratorAsync(request,
        [&bat, &firstShardIterDesc](const auto& outcome) {
          auto guard = makePostGuard(bat);
          EXPECT_TRUE(outcome.IsSuccess());
          firstShardIterDesc = outcome.GetResult();
        }
      );
      context.loopOnBaton(bat);
    }
    const auto& props = firstShardIterDesc.getProperties();
    EXPECT_EQ(context.kStreamName, props.streamName.value());
    EXPECT_EQ(context.firstShardID, props.shardID.value());
    EXPECT_EQ(ShardIteratorType::TRIM_HORIZON, props.iteratorType);
    {
      ThreadBaton bat;
      context.getClient()->getRecordsAsync(
        firstShardIterDesc.getIteratorID(),
        10,
        [&bat](const auto& outcome) {
          auto guard = makePostGuard(bat);
          EXPECT_TRUE(outcome.IsSuccess());
          auto records = outcome.GetResult().GetRecords();
          EXPECT_EQ(2, records.size());
          EXPECT_EQ("good1", getRecordData(records.at(0)));
          EXPECT_EQ("good2", getRecordData(records.at(1)));
        }
      );
      context.loopOnBaton(bat);
    }
  }
}

TEST(TestKCLKinesisClient, TestRecordCRUDShardIterator1) {
  KinesisRecordCRUDTestContext context;
  {
    EXPECT_EQ(10, context.shards.size());
    auto firstHashKey = context.getStartingHashKey(0);
    auto otherHashKey = context.getStartingHashKey(2);
    vector<tuple<string, string, string>> toPut;
    size_t kNumRecords {20};
    for (size_t i = 0; i < kNumRecords; i++) {
      std::ostringstream goodOss;
      std::ostringstream badOss;
      goodOss << "good-" << i;
      badOss << "bad-" << i;
      toPut.push_back(std::make_tuple(
        firstHashKey, "fish", goodOss.str()
      ));
      toPut.push_back(std::make_tuple(
        otherHashKey, "bears", badOss.str()
      ));
    }
    EXPECT_EQ(kNumRecords * 2, toPut.size());
    context.putRecords(toPut);
    ShardIteratorDescription firstShardIterDesc;
    {
      ThreadBaton bat;
      auto request = GetShardIteratorRequest()
        .WithStreamName(context.kStreamName)
        .WithShardId(context.firstShardID)
        .WithShardIteratorType(ShardIteratorType::TRIM_HORIZON);
      context.getClient()->getShardIteratorAsync(request,
        [&bat, &firstShardIterDesc](const auto& outcome) {
          auto guard = makePostGuard(bat);
          EXPECT_TRUE(outcome.IsSuccess());
          firstShardIterDesc = outcome.GetResult();
        }
      );
      context.loopOnBaton(bat);
    }
    const auto& props = firstShardIterDesc.getProperties();
    EXPECT_EQ(context.kStreamName, props.streamName.value());
    EXPECT_EQ(context.firstShardID, props.shardID.value());
    EXPECT_EQ(ShardIteratorType::TRIM_HORIZON, props.iteratorType);
    auto shardIter = util::createShared<ShardIterator>(
      context.baseContext->client, firstShardIterDesc
    );
    {
      ShardIterator::GetNextBatchRequest request;
      request.limit = 10;
      EXPECT_TRUE(kNumRecords % request.limit == 0);
      std::vector<Record> records;
      bool gotEverything {false};
      while (!gotEverything) {        
        ThreadBaton bat;
        shardIter->getNextBatch(request,
          [&bat, &records, &gotEverything](const auto& outcome) {
            auto guard = makePostGuard(bat);
            EXPECT_TRUE(outcome.IsSuccess());
            const auto& batchRecords = outcome.GetResult().GetRecords();
            for (const auto& rec: batchRecords) {
              records.push_back(rec);
            }
            if (batchRecords.size() == 0) {
              gotEverything = true;
            }
          }
        );
        context.loopOnBaton(bat);
      }
      std::set<string> recordDataSet;
      for (auto& record: records) {
        auto data = getRecordData(record);
        auto count = recordDataSet.count(data);
        EXPECT_EQ(0, count)
          << "duplicate record data: '" << data << "'";
        if (count == 0) {
          recordDataSet.insert(data);
        }
      }
      // make sure we didn't get any repeats.
      EXPECT_EQ(recordDataSet.size(), records.size());
      EXPECT_EQ(kNumRecords, records.size());
    }
  }
}