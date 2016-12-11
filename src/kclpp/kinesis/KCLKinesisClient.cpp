#include "kclpp/kinesis/KCLKinesisClient.h"
#include "kclpp/async/tpool/CallbackTask.h"
#include <aws/core/utils/Outcome.h>
#include <aws/kinesis/KinesisClient.h>
#include <aws/kinesis/model/PutRecordsRequest.h>
#include <aws/kinesis/model/PutRecordsResult.h>
#include <aws/kinesis/model/PutRecordRequest.h>
#include <aws/kinesis/model/PutRecordResult.h>
#include <aws/kinesis/model/GetRecordsRequest.h>
#include <aws/kinesis/model/GetRecordsResult.h>
#include <aws/kinesis/model/DescribeStreamRequest.h>
#include <aws/kinesis/model/DescribeStreamResult.h>
#include <aws/kinesis/model/ListStreamsRequest.h>
#include <aws/kinesis/model/ListStreamsResult.h>
#include <aws/kinesis/model/GetShardIteratorRequest.h>
#include <aws/kinesis/model/GetShardIteratorResult.h>
#include <aws/kinesis/model/ShardIteratorType.h>
#include <aws/kinesis/model/CreateStreamRequest.h>
#include <aws/kinesis/model/DeleteStreamRequest.h>
#include "kclpp/kinesis/ShardIterator.h"

namespace kclpp { namespace kinesis {

using kclpp::async::tpool::CallbackTask;

KCLKinesisClient::KCLKinesisClient(state_ptr_t&& state)
  : state_(std::move(state)){}

KCLKinesisClient* KCLKinesisClient::createNew(KCLKinesisClientParams&& params) {
  auto state = std::make_unique<KCLKinesisClientState>();
  state->params = std::move(params);
  state->client.reset(new aws_client_t{state->params.awsConfig});
  return new KCLKinesisClient(std::move(state));
}

void KCLKinesisClient::listStreamsAsync(list_streams_cb_t callback) {
  state_->params.asyncContext->runInEventThread([this, callback]() {
    LOG(INFO) << "listStreamsAsync";
    auto outcomePtr = std::make_shared<ListStreamsOutcome>();
    state_->params.asyncContext->getThreadPool()->trySubmit(
      CallbackTask::createFromEventThread(
        state_->params.asyncContext->getEventContext(),
        [this, outcomePtr]() {
          auto outcome = state_->client->ListStreams(ListStreamsRequest{});
          *outcomePtr = outcome;
          return CallbackTask::done_outcome_t{ Unit{} };
        },
        [this, callback, outcomePtr](const auto&) {
          callback(*outcomePtr);
        }
      )
    );
  });
}


void KCLKinesisClient::describeStreamAsync(const std::string& streamName, describe_stream_cb_t callback) {
  std::string nameCopy = streamName;
  state_->params.asyncContext->runInEventThread([this, callback, nameCopy]() {
    auto outcomePtr = std::make_shared<DescribeStreamOutcome>();
    state_->params.asyncContext->getThreadPool()->trySubmit(
      CallbackTask::createFromEventThread(
        state_->params.asyncContext->getEventContext(),
        [this, nameCopy, outcomePtr]() {
          auto outcome = state_->client->DescribeStream(
            DescribeStreamRequest().WithStreamName(nameCopy)
          );
          *outcomePtr = outcome;
          return CallbackTask::done_outcome_t{ Unit{} };
        },
        [this, callback, outcomePtr](const auto&) {
          callback(*outcomePtr);
        }
      )
    );
  });
}


void KCLKinesisClient::getShardIteratorAsync(const GetShardIteratorRequest& request,
    get_shard_iterator_cb_t callback) {
  GetShardIteratorRequest reqCopy = request;
  state_->params.asyncContext->runInEventThread([this, callback, reqCopy]() {
    auto outcomePtr = std::make_shared<get_shard_iterator_outcome_t>();
    state_->params.asyncContext->getThreadPool()->trySubmit(
      CallbackTask::createFromEventThread(
        state_->params.asyncContext->getEventContext(),
        [this, reqCopy, outcomePtr]() {
          auto outcome = state_->client->GetShardIterator(reqCopy);
          if (!outcome.IsSuccess()) {
            *outcomePtr = get_shard_iterator_outcome_t {outcome.GetError()};
          } else {
            ShardIteratorProperties props;
            props.streamName = StreamName {reqCopy.GetStreamName()};
            props.shardID = ShardID {reqCopy.GetShardId() };
            props.iteratorType = reqCopy.GetShardIteratorType();
            *outcomePtr = get_shard_iterator_outcome_t {
              ShardIteratorDescription {
                ShardIteratorID {outcome.GetResult().GetShardIterator()},
                std::move(props)
              }
            };
          }
          return CallbackTask::done_outcome_t{ Unit{} };
        },
        [this, callback, outcomePtr](const auto&) {
          callback(*outcomePtr);
        }
      )
    );
  });

}


void KCLKinesisClient::getRecordsAsync(const ShardIteratorID& iterId,
    int limit, get_records_cb_t callback) {
  GetRecordsRequest request;
  request.SetShardIterator(iterId.value());
  request.SetLimit(limit);
  state_->params.asyncContext->runInEventThread([this, callback, request]() {
    auto outcomePtr = std::make_shared<GetRecordsOutcome>();
    state_->params.asyncContext->getThreadPool()->trySubmit(
      CallbackTask::createFromEventThread(
        state_->params.asyncContext->getEventContext(),
        [this, request, outcomePtr]() {
          *outcomePtr = state_->client->GetRecords(request);
          return CallbackTask::done_outcome_t{ Unit{} };
        },
        [this, callback, outcomePtr](const auto&) {
          callback(*outcomePtr);
        }
      )
    );
  });
}

void KCLKinesisClient::createStreamAsync(const CreateStreamRequest& request,
    create_stream_cb_t callback) {
  CreateStreamRequest reqCopy = request;
  state_->params.asyncContext->runInEventThread(
    [this, callback, reqCopy]() {
      auto outcomePtr = std::make_shared<create_stream_outcome_t>();
      state_->params.asyncContext->getThreadPool()->trySubmit(
        CallbackTask::createFromEventThread(
          state_->params.asyncContext->getEventContext(),
          [this, reqCopy, outcomePtr]() {
            *outcomePtr = state_->client->CreateStream(reqCopy);
            return CallbackTask::done_outcome_t{ Unit{} };
          },
          [this, callback, outcomePtr](const auto&) {
            callback(*outcomePtr);
          }
        )
      );
    }
  );
}

void KCLKinesisClient::deleteStreamAsync(const DeleteStreamRequest& request,
    delete_stream_cb_t callback) {
  DeleteStreamRequest reqCopy = request;
  state_->params.asyncContext->runInEventThread(
    [this, callback, reqCopy]() {
      auto outcomePtr = std::make_shared<delete_stream_outcome_t>();
      state_->params.asyncContext->getThreadPool()->trySubmit(
        CallbackTask::createFromEventThread(
          state_->params.asyncContext->getEventContext(),
          [this, reqCopy, outcomePtr]() {
            *outcomePtr = state_->client->DeleteStream(reqCopy);
            return CallbackTask::done_outcome_t{ Unit{} };
          },
          [this, callback, outcomePtr](const auto&) {
            callback(*outcomePtr);
          }
        )
      );
    }
  );
}

void KCLKinesisClient::putRecordAsync(const PutRecordRequest& request,
    put_record_cb_t callback) {
  PutRecordRequest reqCopy = request;
  state_->params.asyncContext->runInEventThread(
    [this, callback, reqCopy]() {
      auto outcomePtr = std::make_shared<put_record_outcome_t>();
      state_->params.asyncContext->getThreadPool()->trySubmit(
        CallbackTask::createFromEventThread(
          state_->params.asyncContext->getEventContext(),
          [this, reqCopy, outcomePtr]() {
            *outcomePtr = state_->client->PutRecord(reqCopy);
            return CallbackTask::done_outcome_t{ Unit{} };
          },
          [this, callback, outcomePtr](const auto&) {
            callback(*outcomePtr);
          }
        )
      );
    }
  );
}

void KCLKinesisClient::putRecordsAsync(const PutRecordsRequest& request,
    put_records_cb_t callback) {
  PutRecordsRequest reqCopy = request;
  state_->params.asyncContext->runInEventThread(
    [this, callback, reqCopy]() {
      auto outcomePtr = std::make_shared<put_records_outcome_t>();
      state_->params.asyncContext->getThreadPool()->trySubmit(
        CallbackTask::createFromEventThread(
          state_->params.asyncContext->getEventContext(),
          [this, reqCopy, outcomePtr]() {
            *outcomePtr = state_->client->PutRecords(reqCopy);
            return CallbackTask::done_outcome_t{ Unit{} };
          },
          [this, callback, outcomePtr](const auto&) {
            callback(*outcomePtr);
          }
        )
      );
    }
  );
}

void KCLKinesisClient::listShardsAsync(const StreamName& streamName,
    list_shards_cb_t callback) {
  describeStreamAsync(streamName.value(),
    [this, callback](const auto& outcome) {
      if (!outcome.IsSuccess()) {
        callback(list_shards_outcome_t{outcome.GetError()});
        return;
      }
      auto desc = outcome.GetResult().GetStreamDescription();
      ListShardsResult result;
      result.shards = desc.GetShards();
      callback(list_shards_outcome_t{std::move(result)});
    }
  );
}


}} // kclpp::kinesis
