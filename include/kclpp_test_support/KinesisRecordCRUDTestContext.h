#pragma once

#include <memory>
#include <vector>
#include <string>
#include <initializer_list>
#include <aws/kinesis/model/Shard.h>
#include "kclpp_test_support/StreamGuard.h"
#include "kclpp_test_support/KinesisClientTestContext.h"
#include "kclpp/kinesis/KCLKinesisClient.h"



namespace kclpp_test_support {

struct KinesisRecordCRUDTestContext {
  using KCLKinesisClient = kclpp::kinesis::KCLKinesisClient;
  using Shard = Aws::Kinesis::Model::Shard;
  using PutRecordsRequestEntry = Aws::Kinesis::Model::PutRecordsRequestEntry;

  std::shared_ptr<KinesisClientTestContext> baseContext {nullptr};
  const std::string kStreamName {"some-stream"};
  std::vector<Shard> shards;
  std::string firstShardID;
  std::shared_ptr<StreamGuard> streamGuard;

  KinesisRecordCRUDTestContext(std::shared_ptr<KinesisClientTestContext>);
  KinesisRecordCRUDTestContext();
  ~KinesisRecordCRUDTestContext();
  void putRecords(std::vector<PutRecordsRequestEntry>& entries);
  using rec_tuple_t = std::tuple<std::string, std::string, std::string>;
  void putRecords(const std::vector<rec_tuple_t>& tupEntries);
  void putRecords(std::initializer_list<rec_tuple_t>&& initList);
  KCLKinesisClient* getClient();

  template<typename ...Types>
  void loopOnBaton(Types&&... args) {
    baseContext->loopOnBaton(std::forward<Types>(args)...);
  }

  std::string getStartingHashKey(size_t shardNo);
};

} // kclpp_test_support
