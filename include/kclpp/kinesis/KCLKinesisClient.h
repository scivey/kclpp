#pragma once
#include <aws/kinesis/KinesisClient.h>
#include <aws/kinesis/model/Shard.h>
#include "kclpp/kinesis/types.h"
#include "kclpp/kinesis/ShardIteratorDescription.h"
#include "kclpp/async/KCLAsyncContext.h"

namespace kclpp { namespace kinesis {

class ShardIterator;

class KCLKinesisClient {
 public:
  using aws_client_t = Aws::Kinesis::KinesisClient;
  using client_ptr_t = std::unique_ptr<aws_client_t>;
  using ListStreamsRequest = Aws::Kinesis::Model::ListStreamsRequest;
  using ListStreamsResult = Aws::Kinesis::Model::ListStreamsResult;
  using ListStreamsOutcome = Aws::Kinesis::Model::ListStreamsOutcome;
  using GetShardIteratorRequest = Aws::Kinesis::Model::GetShardIteratorRequest;
  using GetShardIteratorResult = Aws::Kinesis::Model::GetShardIteratorResult;
  using GetShardIteratorOutcome = Aws::Kinesis::Model::GetShardIteratorOutcome;

  using Shard = Aws::Kinesis::Model::Shard;
  using GetRecordsRequest = Aws::Kinesis::Model::GetRecordsRequest;
  using GetRecordsResult = Aws::Kinesis::Model::GetRecordsResult;
  using GetRecordsOutcome = Aws::Kinesis::Model::GetRecordsOutcome;
  using DescribeStreamRequest = Aws::Kinesis::Model::DescribeStreamRequest;
  using DescribeStreamResult = Aws::Kinesis::Model::DescribeStreamResult;
  using DescribeStreamOutcome = Aws::Kinesis::Model::DescribeStreamOutcome;
  using CreateStreamRequest = Aws::Kinesis::Model::CreateStreamRequest;
  using CreateStreamOutcome = Aws::Kinesis::Model::CreateStreamOutcome;
  using DeleteStreamRequest = Aws::Kinesis::Model::DeleteStreamRequest;
  using DeleteStreamOutcome = Aws::Kinesis::Model::DeleteStreamOutcome;
  using PutRecordsRequest = Aws::Kinesis::Model::PutRecordsRequest;
  using PutRecordsOutcome = Aws::Kinesis::Model::PutRecordsOutcome;
  using PutRecordRequest = Aws::Kinesis::Model::PutRecordRequest;
  using PutRecordOutcome = Aws::Kinesis::Model::PutRecordOutcome;

  using aws_errors_t = Aws::Client::AWSError<Aws::Kinesis::KinesisErrors>;

  struct KCLKinesisClientParams {
    std::shared_ptr<async::KCLAsyncContext> asyncContext {nullptr};
    Aws::Client::ClientConfiguration awsConfig;
  };

  struct KCLKinesisClientState {
    KCLKinesisClientParams params;
    client_ptr_t client;
  };
  using state_ptr_t = std::unique_ptr<KCLKinesisClientState>;
 protected:
  state_ptr_t state_ {nullptr};
  KCLKinesisClient(state_ptr_t&& state);
 public:
  static KCLKinesisClient* createNew(KCLKinesisClientParams&& params);


  using list_streams_cb_t = std::function<void(const ListStreamsOutcome&)>;
  void listStreamsAsync(list_streams_cb_t callback);
  using describe_stream_outcome_t = DescribeStreamOutcome;
  using describe_stream_cb_t = std::function<void(const describe_stream_outcome_t&)>;
  void describeStreamAsync(const std::string& streamName, describe_stream_cb_t callback);

  using get_shard_iterator_outcome_t = Aws::Utils::Outcome<
    ShardIteratorDescription, aws_errors_t
  >;
  using get_shard_iterator_cb_t = std::function<void(const get_shard_iterator_outcome_t&)>;
  void getShardIteratorAsync(const GetShardIteratorRequest& request,
    get_shard_iterator_cb_t callback);

  using get_records_cb_t = std::function<void(const GetRecordsOutcome&)>;
  void getRecordsAsync(const ShardIteratorID& iterId, int limit, get_records_cb_t callback);

  struct ListShardsResult {
    Aws::Vector<Shard> shards;
  };

  using create_stream_outcome_t = CreateStreamOutcome;
  using create_stream_cb_t = std::function<void(const CreateStreamOutcome&)>;
  void createStreamAsync(const CreateStreamRequest& request, create_stream_cb_t callback);

  using delete_stream_outcome_t = DeleteStreamOutcome;
  using delete_stream_cb_t = std::function<void(const delete_stream_outcome_t&)>;
  void deleteStreamAsync(const DeleteStreamRequest& request, delete_stream_cb_t callback);

  using list_shards_outcome_t = Outcome<ListShardsResult, aws_errors_t>;
  using list_shards_cb_t = std::function<void(const list_shards_outcome_t&)>;
  void listShardsAsync(const StreamName& streamName, list_shards_cb_t);

  using put_records_outcome_t = PutRecordsOutcome;
  using put_records_cb_t = std::function<void(const put_records_outcome_t&)>;
  void putRecordsAsync(const PutRecordsRequest&, put_records_cb_t);

  using put_record_outcome_t = PutRecordOutcome;
  using put_record_cb_t = std::function<void(const put_record_outcome_t&)>;
  void putRecordAsync(const PutRecordRequest&, put_record_cb_t);
};

}} // kclpp::kinesis
