#include <glog/logging.h>
#include <chrono>
#include <thread>

#include <typeinfo>
#include <aws/core/utils/DateTime.h>
#include <aws/core/Aws.h>

#include <aws/dynamodb/DynamoDBClient.h>
#include <aws/dynamodb/model/ListTablesRequest.h>
#include <aws/dynamodb/model/ListTablesResult.h>
#include <aws/dynamodb/model/DescribeTableRequest.h>
#include <aws/dynamodb/model/DescribeTableResult.h>
#include <aws/dynamodb/model/GetItemRequest.h>
#include <aws/dynamodb/model/GetItemResult.h>
#include <aws/dynamodb/model/TableDescription.h>
#include <aws/core/utils/Outcome.h>

#include "kclpp/APIGuard.h"

using namespace std;
using Aws::DynamoDB::DynamoDBClient;
using Aws::DynamoDB::Model::ListTablesResult;
using Aws::DynamoDB::Model::ListTablesRequest;
using Aws::DynamoDB::Model::DescribeTableRequest;
using Aws::DynamoDB::Model::DescribeTableResult;
using Aws::DynamoDB::Model::GetItemResult;
using Aws::DynamoDB::Model::GetItemRequest;
using Aws::DynamoDB::Model::AttributeValue;







void doDynamo() {
  DynamoDBClient client;
  auto showTables = [&client]() {
    ListTablesRequest request;
    auto outcome = client.ListTables(request);
    CHECK(outcome.IsSuccess());
    auto result = outcome.GetResultWithOwnership();
    auto names = result.GetTableNames();
    for (const auto& name: names) {
      LOG(INFO) << "TABLE: " << name;
    }
    LOG(INFO) << "done";
  };
  auto descTable = [&client]() {
    DescribeTableRequest descTable;
    descTable.SetTableName("test_1");
    auto descOutcome = client.DescribeTable(descTable);
    CHECK(descOutcome.IsSuccess());
    auto descRes = descOutcome.GetResultWithOwnership();
    auto table = descRes.GetTable();
    auto readable = table.Jsonize().WriteReadable();
    LOG(INFO) << "result: " << readable;
  };
  auto getItem = [&client]() {
    GetItemRequest request;
    request.SetTableName("test_1");
    Aws::Map<Aws::String, AttributeValue> keyMap {
      {"leaseKey", AttributeValue{"something"}}
    };
    request.SetKey(keyMap);
    auto outcome = client.GetItem(request);
    CHECK(outcome.IsSuccess());
    auto result = outcome.GetResultWithOwnership();
    auto item = result.GetItem();
    for (auto& attrPair: item) {
      LOG(INFO) << attrPair.first;
    }
  };
  descTable();
  getItem();
}

int main() {
  google::InstallFailureSignalHandler();
  LOG(INFO) << "start";
  kclpp::APIGuardFactory factory;
  factory.getOptions().loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Debug;
  auto guard = factory.build();
  doDynamo();
  LOG(INFO) << "end";
}
