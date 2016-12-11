#pragma once

#include "kclpp/dynamo/KCLDynamoClient.h"
#include "kclpp/leases/Lease.h"
#include "kclpp/async/KCLAsyncContext.h"

#include "kclpp/dynamo/types.h"
#include "kclpp/leases/dynamo_helpers.h"

namespace kclpp { namespace leases {

class LeaseManagerIf {
 public:
  using dynamo_client_t = kclpp::dynamo::KCLDynamoClient;
  using dynamo_client_ptr_t = std::shared_ptr<dynamo_client_t>;
  using aws_errors_t = Aws::Client::AWSError<Aws::DynamoDB::DynamoDBErrors>;
  using lease_ptr_t = typename dynamo_helpers::lease_ptr_t;
  using KCLDynamoClient = dynamo::KCLDynamoClient;
  using AttributeValue = Aws::DynamoDB::Model::AttributeValue;
  using ExpectedAttributeValue = Aws::DynamoDB::Model::ExpectedAttributeValue;
  using GetItemRequest = Aws::DynamoDB::Model::GetItemRequest;
  using GetItemOutcome = Aws::DynamoDB::Model::GetItemOutcome;
  using PutItemRequest = Aws::DynamoDB::Model::PutItemRequest;
  using PutItemOutcome = Aws::DynamoDB::Model::PutItemOutcome;
  using UpdateItemRequest = Aws::DynamoDB::Model::UpdateItemRequest;
  using UpdateItemOutcome = Aws::DynamoDB::Model::UpdateItemOutcome;
  using ScanRequest = Aws::DynamoDB::Model::ScanRequest;
  using ScanOutcome = Aws::DynamoDB::Model::ScanOutcome;
  using DeleteItemRequest = Aws::DynamoDB::Model::DeleteItemRequest;
  using DeleteItemOutcome = Aws::DynamoDB::Model::DeleteItemOutcome;

  using CreateTableRequest = Aws::DynamoDB::Model::CreateTableRequest;
  using CreateTableOutcome = Aws::DynamoDB::Model::CreateTableOutcome;
  using KeySchemaElement = Aws::DynamoDB::Model::KeySchemaElement;
  using KeyType = Aws::DynamoDB::Model::KeyType;
  using AttributeValueUpdate = Aws::DynamoDB::Model::AttributeValueUpdate;
  using ProvisionedThroughput = Aws::DynamoDB::Model::ProvisionedThroughput;
  using AttributeDefinition = Aws::DynamoDB::Model::AttributeDefinition;
  using ScalarAttributeType = Aws::DynamoDB::Model::ScalarAttributeType;
  using DynamoDBErrors = Aws::DynamoDB::DynamoDBErrors;


  using create_shared_outcome_t = Aws::Utils::Outcome<
    std::shared_ptr<LeaseManagerIf>, kclpp::InvalidState
  >;

  struct CreateTableParams {
    size_t readCapacity {10};
    size_t writeCapacity {10};
  };

  struct CreateTableIfNotExistsResult {
    bool created {false};
  };

  using create_table_outcome_t = Aws::Utils::Outcome<CreateTableIfNotExistsResult, aws_errors_t>;
  using create_table_cb_t = std::function<void (const create_table_outcome_t&)>;
  virtual void createTableIfNotExists(const CreateTableParams&, create_table_cb_t) = 0;

  using list_leases_outcome_t = Aws::Utils::Outcome<std::vector<lease_ptr_t>, aws_errors_t>;
  using list_leases_cb_t = std::function<void (const list_leases_outcome_t&)>;
  virtual void listLeases(list_leases_cb_t callback) = 0;

  struct DeleteLeaseResult {
    bool deleted {false};
  };

  using delete_lease_outcome_t = Aws::Utils::Outcome<DeleteLeaseResult, aws_errors_t>;
  using delete_lease_cb_t = std::function<void (const delete_lease_outcome_t&)>;

  virtual void deleteLeaseCAS(lease_ptr_t lease, delete_lease_cb_t callback) = 0;
  virtual void deleteLeaseUnconditionally(lease_ptr_t lease, delete_lease_cb_t callback) = 0;

  struct UpdateLeaseResult {
    bool updated {false};
    lease_ptr_t lease {nullptr};
  };

  using update_lease_outcome_t = Aws::Utils::Outcome<UpdateLeaseResult, aws_errors_t>;
  using update_lease_cb_t = std::function<void (const update_lease_outcome_t&)>;

  // internalUpdateLease doesn't quite match what the Java KCL does for `updateLease`.
  // instead, keeping duplication for now.
  virtual void updateLease(lease_ptr_t lease, update_lease_cb_t callback) = 0;
  struct IsLeaseTableEmptyResult {
    bool isEmpty {false};
  };

  using is_table_empty_outcome_t = Aws::Utils::Outcome<IsLeaseTableEmptyResult, aws_errors_t>;
  using is_table_empty_cb_t = std::function<void (const is_table_empty_outcome_t&)>;
  virtual void isLeaseTableEmpty(is_table_empty_cb_t callback) = 0;

  struct CreateLeaseIfNotExistsResult {
    bool created {false};
    Optional<lease_ptr_t> lease;
  };

  using create_lease_outcome_t = Aws::Utils::Outcome<CreateLeaseIfNotExistsResult, aws_errors_t>;
  using create_lease_cb_t = std::function<void(const create_lease_outcome_t&)>;
  virtual void createLeaseIfNotExists(lease_ptr_t lease, create_lease_cb_t callback) = 0;

  struct GetLeaseResult {
    lease_ptr_t lease {nullptr};
  };
  // this may need a wrapping error type: does aws_errors_t include a failed GetItemRequest?
  using get_lease_outcome_t = Aws::Utils::Outcome<GetLeaseResult, aws_errors_t>;
  using get_lease_cb_t = std::function<void(const get_lease_outcome_t&)>;
  virtual void getLease(LeaseKey key, get_lease_cb_t callback) = 0;

  struct RenewLeaseResult {
    bool renewed {false};
    lease_ptr_t lease {nullptr};
  };

  using renew_lease_outcome_t = Aws::Utils::Outcome<RenewLeaseResult, aws_errors_t>;
  using renew_lease_cb_t = std::function<void(const renew_lease_outcome_t&)>;
  virtual void renewLease(lease_ptr_t lease, renew_lease_cb_t callback) = 0;

  struct EvictLeaseResult {
    bool evicted {false};
  };


  using evict_lease_outcome_t = Aws::Utils::Outcome<EvictLeaseResult, aws_errors_t>;
  using evict_lease_cb_t = std::function<void(const evict_lease_outcome_t&)>;
  virtual void evictLease(lease_ptr_t lease, evict_lease_cb_t callback) = 0;

  struct TakeLeaseResult {
    bool taken {false};
    lease_ptr_t lease {nullptr};
  };
  using take_lease_outcome_t = Aws::Utils::Outcome<TakeLeaseResult, aws_errors_t>;
  using take_lease_cb_t = std::function<void(const take_lease_outcome_t&)>;
  virtual void takeLease(lease_ptr_t lease, LeaseOwner newOwner, take_lease_cb_t callback) = 0;

  struct WaitUntilTableExistsParams {
    using msec = std::chrono::milliseconds;
    msec pollInterval {10000};
    msec timeout {600000};
  };
  
  using wait_until_exists_outcome_t = Aws::Utils::Outcome<
    KCLDynamoClient::DoesTableExistResult,
    aws_errors_t
  >;
  using wait_until_exists_cb_t = std::function<void(const wait_until_exists_outcome_t&)>;
  virtual void waitUntilLeaseTableExists(const WaitUntilTableExistsParams&, wait_until_exists_cb_t) = 0;
  void waitUntilLeaseTableExists(wait_until_exists_cb_t callback) {
    waitUntilLeaseTableExists(WaitUntilTableExistsParams(), callback);
  }

  virtual ~LeaseManagerIf() = default;
};

}} // kclpp::leases