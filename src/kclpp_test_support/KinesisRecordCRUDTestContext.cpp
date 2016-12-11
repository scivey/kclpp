#include "kclpp_test_support/KinesisRecordCRUDTestContext.h"
#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <string>
#include <set>
#include <aws/kinesis/model/Shard.h>
#include <aws/kinesis/model/PutRecordsRequestEntry.h>
#include <aws/kinesis/model/PutRecordsRequest.h>

#include "kclpp_test_support/StreamGuard.h"
#include "kclpp_test_support/RecordInput.h"
#include "kclpp_test_support/KinesisClientTestContext.h"
#include "kclpp_test_support/misc_kinesis.h"
#include "kclpp_test_support/misc.h"

using namespace std;
using Aws::Kinesis::Model::Shard;
using Aws::Kinesis::Model::PutRecordsRequest;
using Aws::Kinesis::Model::PutRecordsRequestEntry;

using kclpp::kinesis::StreamName;
using kclpp::kinesis::KCLKinesisClient;
using kclpp::locks::ThreadBaton;

namespace kclpp_test_support {

KinesisRecordCRUDTestContext::KinesisRecordCRUDTestContext()
  : KinesisRecordCRUDTestContext(std::make_shared<KinesisClientTestContext>()) {}


KinesisRecordCRUDTestContext::KinesisRecordCRUDTestContext(
    shared_ptr<KinesisClientTestContext> base) {
  baseContext = base;
  streamGuard = std::shared_ptr<StreamGuard> {
    new StreamGuard {
      StreamGuard::create(
        baseContext.get(),
        StreamGuardOptions().withName(kStreamName)
     )
    }
  };
  {
    ThreadBaton bat;
    baseContext->client->listShardsAsync(StreamName{kStreamName},
      [this, &bat](const auto& outcome) {
        auto guard = makePostGuard(bat);
        EXPECT_TRUE(outcome.IsSuccess());
        auto newShards = outcome.GetResult().shards;
        EXPECT_TRUE(newShards.size() > 0);
        this->shards = newShards;
        this->firstShardID = newShards.at(0).GetShardId();
      }
    );
    baseContext->loopOnBaton(bat);      
  }
}

KinesisRecordCRUDTestContext::~KinesisRecordCRUDTestContext() {
  streamGuard.reset();
  baseContext.reset();
}

void KinesisRecordCRUDTestContext::putRecords(vector<PutRecordsRequestEntry>& entries) {
  {
    set<string> dataSet;
    for (const auto& entry: entries) {
      auto data = getRecordData(entry.GetData());
      CHECK(dataSet.count(data) == 0);
      dataSet.insert(data);
    }      
  }

  ThreadBaton bat;
  PutRecordsRequest request;
  request.SetStreamName(kStreamName);
  request.SetRecords(entries);
  baseContext->client->putRecordsAsync(request,
    [&bat](const auto& outcome) {
      auto guard = makePostGuard(bat);
      EXPECT_TRUE(outcome.IsSuccess())
        << "putRecordsAsync error: "
        << outcome.GetError().GetExceptionName()
        << " : '" << outcome.GetError().GetMessage() << "'";
    }
  );
  baseContext->loopOnBaton(bat);
}

void KinesisRecordCRUDTestContext::putRecords(const vector<rec_tuple_t>& tupEntries) {
  vector<PutRecordsRequestEntry> entries;
  for (const auto& item: tupEntries) {
    const auto& explicitKey = std::get<0>(item);
    const auto& partitionKey = std::get<1>(item);
    const auto& data = std::get<2>(item);
    auto rec = RecordInput(partitionKey, data).toRequestEntry();
    rec.SetExplicitHashKey(explicitKey);
    entries.emplace_back(std::move(rec));
  }
  putRecords(entries);    
}

void KinesisRecordCRUDTestContext::putRecords(initializer_list<rec_tuple_t>&& initList) {
  vector<rec_tuple_t> entries { 
    std::forward<initializer_list<rec_tuple_t>>(initList)
  };
  putRecords(entries);
}

KCLKinesisClient* KinesisRecordCRUDTestContext::getClient() {
  return baseContext->client.get();
}

string KinesisRecordCRUDTestContext::getStartingHashKey(size_t shardNo) {
  EXPECT_TRUE(shardNo < shards.size());
  auto& shard = shards.at(shardNo);
  return shard.GetHashKeyRange().GetStartingHashKey();
}

} // kclpp_test_support
