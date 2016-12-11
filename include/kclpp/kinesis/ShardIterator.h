#pragma once
#include "kclpp/kinesis/types.h"
#include "kclpp/kinesis/KCLKinesisClient.h"
#include "kclpp/Optional.h"
#include "kclpp/kinesis/ShardIteratorProperties.h"
#include "kclpp/kinesis/ShardIteratorDescription.h"

namespace kclpp { namespace kinesis {

class ShardIterator {
 public:
  using client_t = KCLKinesisClient;
  using client_ptr_t = std::shared_ptr<client_t>;
  using client_errors_t = typename client_t::aws_errors_t;
  using GetRecordsRequest = Aws::Kinesis::Model::GetRecordsRequest;
  using GetRecordsOutcome = Aws::Kinesis::Model::GetRecordsOutcome;
  struct ShardIteratorState {
    client_ptr_t kinesisClient {nullptr};
    ShardIteratorProperties properties;
    ShardIteratorID currentID;
    Optional<ShardIteratorID> nextID;
  };
 protected:
  ShardIteratorState iterState_;
  ShardIterator(ShardIteratorState&& iterState);
 public:
  static ShardIterator* createNew(client_ptr_t, const ShardIteratorDescription&) noexcept;
  const ShardIteratorState& getState() const noexcept;
  bool isAtEnd() const noexcept;

  struct GetNextBatchRequest {
    size_t limit {500};    
  };
  using get_next_batch_outcome_t = GetRecordsOutcome;
  using get_next_batch_cb_t = std::function<void (const get_next_batch_outcome_t&)>;

  void getNextBatch(const GetNextBatchRequest&, get_next_batch_cb_t) noexcept;
  std::ostream& pprint(std::ostream& oss) const noexcept;
  std::string pprint() const noexcept;
};

}} // kclpp::kinesis
