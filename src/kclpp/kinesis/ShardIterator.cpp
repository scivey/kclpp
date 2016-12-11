#include "kclpp/kinesis/ShardIterator.h"
#include <aws/kinesis/KinesisClient.h>
#include <aws/kinesis/model/PutRecordsRequest.h>
#include <aws/kinesis/model/PutRecordsResult.h>
#include <aws/kinesis/model/GetRecordsRequest.h>
#include <aws/kinesis/model/GetRecordsResult.h>
#include <aws/kinesis/model/DescribeStreamRequest.h>
#include <aws/kinesis/model/DescribeStreamResult.h>
#include <aws/kinesis/model/ListStreamsRequest.h>
#include <aws/kinesis/model/ListStreamsResult.h>
#include <aws/kinesis/model/GetShardIteratorRequest.h>
#include <aws/kinesis/model/GetShardIteratorResult.h>
#include <aws/kinesis/model/ShardIteratorType.h>
#include "kclpp/Optional.h"
#include "kclpp/kinesis/ShardIteratorProperties.h"
#include "kclpp/kinesis/ShardIteratorDescription.h"

namespace kclpp { namespace kinesis {

using ShardIteratorState = ShardIterator::ShardIteratorState;

ShardIterator::ShardIterator(ShardIteratorState&& iterState)
  : iterState_(std::move(iterState)) {}

ShardIterator* ShardIterator::createNew(client_ptr_t client,
    const ShardIteratorDescription& description) noexcept {
  ShardIteratorState iterState;
  iterState.properties = description.getProperties();
  iterState.nextID.assign(description.getIteratorID());
  iterState.kinesisClient = client;
  return new ShardIterator(std::move(iterState));
}

const ShardIteratorState& ShardIterator::getState() const noexcept {
  return iterState_;
}

bool ShardIterator::isAtEnd() const noexcept {
  return !iterState_.nextID.hasValue();
}

void ShardIterator::getNextBatch(const GetNextBatchRequest& request,
    get_next_batch_cb_t callback) noexcept {
  DCHECK(!!iterState_.kinesisClient);
  // TODO: turn "already at end" into a user-level error condition
  CHECK(!isAtEnd());
  iterState_.kinesisClient->getRecordsAsync(
    iterState_.nextID.value(),
    request.limit,
    [this, callback](const auto& outcome) {
      if (!outcome.IsSuccess()) {
        callback(outcome);
        return;
      }
      const auto& awsResult = outcome.GetResult();
      this->iterState_.currentID = this->iterState_.nextID.value();
      auto nextId = awsResult.GetNextShardIterator();
      // docs are fuzzy here.
      // we're looking for the end-of-shard indicator.
      if (nextId.empty() || nextId.find("null") == 0) {
        this->iterState_.nextID.clear();
      } else {
        this->iterState_.nextID.assign(ShardIteratorID {nextId});
      }
      callback(outcome);
    }
  );
}

std::ostream& ShardIterator::pprint(std::ostream& oss) const noexcept {
  const auto& iterState = getState();
  oss << "ShardIterator:\n"
      << "\tcurrentID=" << iterState.currentID.value() << "\n"
      << "\tnextID=";
  if (iterState.nextID.hasValue()) {
    oss << iterState.nextID.value().value();
  } else {
    oss << "NOTHING";
  }
  oss << "\n";
  return oss;
}

std::string ShardIterator::pprint() const noexcept {
  std::ostringstream oss;
  pprint(oss);
  return oss.str();
}

}} // kclpp::kinesis
