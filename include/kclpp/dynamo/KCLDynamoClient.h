#pragma once
#include <memory>
#include <utility>
#include <functional>
#include <aws/dynamodb/DynamoDBClient.h>
#include "kclpp/async/KCLAsyncContext.h"

namespace kclpp { namespace dynamo {

class KCLDynamoClient: public std::enable_shared_from_this<KCLDynamoClient> {
 public:
  using aws_client_t = Aws::DynamoDB::DynamoDBClient;
  using client_ptr_t = std::unique_ptr<aws_client_t>;
  using AsyncCallerContext = Aws::Client::AsyncCallerContext;
  using GetItemRequest = Aws::DynamoDB::Model::GetItemRequest;
  using GetItemResult = Aws::DynamoDB::Model::GetItemResult;
  using GetItemOutcome = Aws::DynamoDB::Model::GetItemOutcome;
  using ScanRequest = Aws::DynamoDB::Model::ScanRequest;
  using ScanOutcome = Aws::DynamoDB::Model::ScanOutcome;
  using ScanResult = Aws::DynamoDB::Model::ScanResult;
  using PutItemRequest = Aws::DynamoDB::Model::PutItemRequest;
  using PutItemResult = Aws::DynamoDB::Model::PutItemResult;
  using PutItemOutcome = Aws::DynamoDB::Model::PutItemOutcome;
  using UpdateItemRequest = Aws::DynamoDB::Model::UpdateItemRequest;
  using UpdateItemOutcome = Aws::DynamoDB::Model::UpdateItemOutcome;
  using DeleteItemOutcome = Aws::DynamoDB::Model::DeleteItemOutcome;
  using DeleteItemRequest = Aws::DynamoDB::Model::DeleteItemRequest;
  using CreateTableRequest = Aws::DynamoDB::Model::CreateTableRequest;
  using CreateTableOutcome = Aws::DynamoDB::Model::CreateTableOutcome;
  using ListTablesRequest = Aws::DynamoDB::Model::ListTablesRequest;
  using ListTablesOutcome = Aws::DynamoDB::Model::ListTablesOutcome;
  using DeleteTableRequest = Aws::DynamoDB::Model::DeleteTableRequest;
  using DeleteTableOutcome = Aws::DynamoDB::Model::DeleteTableOutcome;
  using DescribeTableRequest = Aws::DynamoDB::Model::DescribeTableRequest;
  using DescribeTableOutcome = Aws::DynamoDB::Model::DescribeTableOutcome;
  using DynamoDBErrors = Aws::DynamoDB::DynamoDBErrors;
  using aws_errors_t = Aws::Client::AWSError<Aws::DynamoDB::DynamoDBErrors>;


  struct KCLDynamoClientParams {
    std::shared_ptr<async::KCLAsyncContext> asyncContext {nullptr};
    Aws::Client::ClientConfiguration awsConfig;
  };

  struct KCLDynamoClientState {
    KCLDynamoClientParams params;
    client_ptr_t client;
  };

  using state_ptr_t = std::unique_ptr<KCLDynamoClientState>;
 protected:
  state_ptr_t state_ {nullptr};
  KCLDynamoClient(state_ptr_t&& state);

 public:
  static KCLDynamoClient* createNew(KCLDynamoClientParams&& params);

 public:
  using get_item_outcome_t = Aws::DynamoDB::Model::GetItemOutcome;
  using get_item_cb_t = std::function<void (get_item_outcome_t&&)>;
  void getItemAsync(GetItemRequest&& request, get_item_cb_t&& callback);
  void getItemAsync(const GetItemRequest& request, get_item_cb_t&& callback);

  using put_item_outcome_t = Aws::DynamoDB::Model::PutItemOutcome;
  using put_item_cb_t = std::function<void (const put_item_outcome_t&)>;
  void putItemAsync(const PutItemRequest& request, put_item_cb_t callback);

  using update_item_outcome_t = Aws::DynamoDB::Model::UpdateItemOutcome;
  using update_item_cb_t = std::function<void (const update_item_outcome_t&)>;
  void updateItemAsync(const UpdateItemRequest& request, update_item_cb_t callback);

  using delete_item_outcome_t = Aws::DynamoDB::Model::DeleteItemOutcome;
  using delete_item_cb_t = std::function<void (const delete_item_outcome_t&)>;
  void deleteItemAsync(const DeleteItemRequest& request, delete_item_cb_t callback);

  using scan_outcome_t = Aws::DynamoDB::Model::ScanOutcome;
  using scan_cb_t = std::function<void (const scan_outcome_t&)>;
  void scanAsync(const ScanRequest& request, scan_cb_t callback);

  using create_table_outcome_t = CreateTableOutcome;
  using create_table_cb_t = std::function<void (const create_table_outcome_t&)>;
  void createTableAsync(const CreateTableRequest& request, create_table_cb_t callback);

  using list_tables_outcome_t = ListTablesOutcome;
  using list_tables_cb_t = std::function<void (const list_tables_outcome_t&)>;
  void listTablesAsync(const ListTablesRequest& request, list_tables_cb_t callback);

  using delete_table_outcome_t = DeleteTableOutcome;
  using delete_table_cb_t = std::function<void (const delete_table_outcome_t&)>;
  void deleteTableAsync(const DeleteTableRequest& request, delete_table_cb_t callback);

  using describe_table_outcome_t = DescribeTableOutcome;
  using describe_table_cb_t = std::function<void (const describe_table_outcome_t&)>;
  void describeTableAsync(const DescribeTableRequest& request, describe_table_cb_t callback);

  struct DoesTableExistResult {
    bool exists {false};
  };
  using does_table_exist_outcome_t = Aws::Utils::Outcome<DoesTableExistResult, aws_errors_t>;
  using does_table_exist_cb_t = std::function<void (const does_table_exist_outcome_t&)>;
  void doesTableExistAsync(const Aws::String& tableName, does_table_exist_cb_t callback);
};


}} // kclpp::dynamo
