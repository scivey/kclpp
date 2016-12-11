#include <glog/logging.h>
#include <chrono>
#include <thread>

#include <typeinfo>
#include <map>

#include <aws/core/utils/DateTime.h>
#include <aws/core/Aws.h>

#include <aws/dynamodb/DynamoDBClient.h>
#include <aws/dynamodb/model/ListTablesRequest.h>
#include <aws/dynamodb/model/ListTablesResult.h>
#include <aws/dynamodb/model/DescribeTableRequest.h>
#include <aws/dynamodb/model/DescribeTableResult.h>
#include <aws/dynamodb/model/GetItemRequest.h>
#include <aws/dynamodb/model/GetItemResult.h>
#include <aws/dynamodb/model/UpdateItemRequest.h>
#include <aws/dynamodb/model/UpdateItemResult.h>
#include <aws/dynamodb/model/TableDescription.h>
#include <aws/core/utils/Outcome.h>
#include <aws/core/utils/threading/Executor.h>



using namespace std;
using Aws::DynamoDB::DynamoDBClient;
using Aws::DynamoDB::Model::ListTablesResult;
using Aws::DynamoDB::Model::ListTablesRequest;
using Aws::DynamoDB::Model::DescribeTableRequest;
using Aws::DynamoDB::Model::DescribeTableResult;
using Aws::DynamoDB::Model::GetItemResult;
using Aws::DynamoDB::Model::GetItemRequest;
using Aws::DynamoDB::Model::AttributeValue;
using Aws::DynamoDB::Model::ExpectedAttributeValue;
using Aws::DynamoDB::Model::AttributeValueUpdate;
using Aws::DynamoDB::Model::UpdateItemRequest;
using Aws::DynamoDB::Model::UpdateItemResult;



#include "kclpp/APIGuard.h"
#include "kclpp/macros.h"
#include "kclpp/leases/Lease.h"
#include "kclpp/kinesis/KCLPPKinesisClient.h"
#include "kclpp/kinesis/ShardIterator.h"
#include "kclpp/dynamo/KCLPPDynamoClient.h"
#include "kclpp/clients/KCLPPClientFactory.h"
#include "kclpp/leases/LeaseManager.h"
#include "kclpp/leases/LeaseRenewer.h"
#include "kclpp/locks/ThreadBaton.h"
using kclpp::locks::ThreadBaton;


using kclpp::dynamo::KCLPPDynamoClient;

template<typename T>
using KCLPPClientFactory = kclpp::clients::KCLPPClientFactory<T>;

using kclpp::kinesis::KCLPPKinesisClient;
using kclpp::kinesis::ShardIterator;
using kclpp::leases::LeaseManager;



AttributeValue intAttr(int64_t x) {
  std::ostringstream oss;
  oss << x;
  AttributeValue attrVal;
  attrVal.SetN(oss.str());
  return attrVal;
}

ExpectedAttributeValue expectedIntAttr(int64_t x) {
  ExpectedAttributeValue val;
  val.SetValue(intAttr(x));
  return val;
}


void doConditional(string leaseKey, int64_t expectedCounter,
    const map<string, string>& attrs) {
  DynamoDBClient client;
  UpdateItemRequest request;
  Aws::Map<Aws::String, AttributeValue> keyMap {
    {"leaseKey", AttributeValue(leaseKey)}
  };
  Aws::Map<Aws::String, ExpectedAttributeValue> expectedValues {
    {"leaseCounter", expectedIntAttr(expectedCounter)}
  };
  Aws::Map<Aws::String, AttributeValueUpdate> newValues;
  for (const auto& item: attrs) {
    AttributeValueUpdate toUpdate;
    AttributeValue attrVal {item.second};
    toUpdate.SetValue(attrVal);
    Aws::String key = item.first;
    std::pair<Aws::String, AttributeValueUpdate> toInsert {
      key, toUpdate
    };
    newValues.insert(toInsert);
  }
  AttributeValueUpdate updatedCounter;
  updatedCounter.SetValue(intAttr(expectedCounter+1));
  newValues.insert(std::make_pair(
    "leaseCounter", updatedCounter
  ));
  request
    .WithTableName("test_1")
    .WithKey(keyMap)
    .WithExpected(expectedValues)
    .WithAttributeUpdates(newValues);
  auto outcome = client.UpdateItem(request);
  KCLPP_CHECK_OUTCOME(outcome);
  auto result = outcome.GetResultWithOwnership();
  LOG(INFO) << "got result!";
}


void moreDynamo() {
  auto easy = kclpp::clients::KCLPPClientFactory<KCLPPDynamoClient>::createShared();
  GetItemRequest request;
  request.SetTableName("CrawlfishFetcher16");
  Aws::Map<Aws::String, AttributeValue> keyMap {
    {"leaseKey", AttributeValue{"shardId-000000000009"}}
  };
  request.SetKey(keyMap);
  ThreadBaton baton;
  // easy->getItemAsync(request, [&baton](const typename KCLPPDynamoClient::get_item_outcome_t& outcome) {
  //   LOG(INFO) << "callback.";
  //   KCLPP_CHECK_OUTCOME(outcome);
  //   auto result = outcome.GetResult();
  //   auto item = result.GetItem();
  //   for (auto& attrPair: item) {
  //     LOG(INFO) << attrPair.first;
  //   }  
  //   auto lease = kclpp::leases::Lease::fromDynamoDict(item);
  //   if (lease.IsSuccess()) {
  //     LOG(INFO) << "deserialize success.";
  //     auto leaseResult = lease.GetResultWithOwnership();
  //     const auto& leaseState = leaseResult.getState();
  //     auto owner = leaseState.leaseOwner.value().value();
  //     LOG(INFO) << "owner: " << owner;
  //     auto checkpoint = leaseState.checkpoint.value().value();
  //     LOG(INFO) << "checkpoint: " << checkpoint;
  //   } else {
  //     LOG(INFO) << "deserialize failure";
  //   }
  //   baton.post();
  // });

  LeaseManager::LeaseManagerState managerState;
  managerState.tableName = kclpp::dynamo::TableName {"CrawlfishFetcher16"};
  managerState.dynamoClient = easy;
  auto leaseManagerOut = LeaseManager::createShared(std::move(managerState));
  CHECK(leaseManagerOut.IsSuccess());
  auto leaseManager = leaseManagerOut.GetResultWithOwnership();
  leaseManager->listLeases([&baton](const LeaseManager::list_leases_outcome_t& outcome) {
    LOG(INFO) << "listed leases.";
    if (outcome.IsSuccess()) {
      LOG(INFO) << "success";
      auto leases = outcome.GetResult();
      for (const auto& lease: leases) {
        auto key = lease->getState().leaseKey.value();
        auto owner = lease->getState().leaseOwner.value().value();
        LOG(INFO) << "key '" << key << "' owned by '" << owner << "'";
      }
    } else {
      LOG(INFO) << "failure.";
    }
    baton.post();
  });
  baton.wait();
  // KCLPPDynamoClient::ScanRequest scanReq;
  // scanReq.SetTableName("CrawlfishFetcher16");
  // scanReq.SetLimit(100);
  // easy->scanAsync(scanReq, [&baton](const KCLPPDynamoClient::scan_outcome_t& outcome) {
  //   LOG(INFO) << "scanned!";
  //   auto items = outcome.GetResult().GetItems();
  //   for (const auto& item: items) {
  //     auto mainAttr = item.find("leaseKey");
  //     CHECK(mainAttr != item.end());
  //     auto asString = mainAttr->second.GetS();
  //     LOG(INFO) << "lease key: " << asString;
  //   }
  //   baton.post();
  // });
  // baton.wait();
}

void lessDynamo() {
  auto easy = kclpp::clients::KCLPPClientFactory<KCLPPDynamoClient>::createShared();
  ThreadBaton baton;
  easy->listTablesAsync(ListTablesRequest(),
    [&baton]
    (const KCLPPDynamoClient::list_tables_outcome_t& outcome) {
      LOG(INFO) << "got outcome.";
      baton.post();
    }
  );
  baton.wait();
}

int main(int argc, char **argv) {
  google::InstallFailureSignalHandler();
  LOG(INFO) << "start";
  kclpp::APIGuardFactory factory;
  factory.getOptions().loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Debug;
  auto guard = factory.build();
  // return mainKinesis(argc, argv);
  // return tryAsync(argc, argv);
  lessDynamo();
}
