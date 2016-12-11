#include "kclpp/dynamo/KCLDynamoClient.h"
#include "kclpp/async/tpool/CallbackTask.h"

#include <glog/logging.h>
#include <memory>
#include <utility>
#include <functional>
#include <aws/dynamodb/DynamoDBClient.h>
#include <aws/dynamodb/DynamoDBErrors.h>
#include <aws/core/client/AWSError.h>
#include <aws/core/utils/Outcome.h>
#include <aws/dynamodb/model/ListTablesRequest.h>
#include <aws/dynamodb/model/ListTablesResult.h>
#include <aws/dynamodb/model/DescribeTableRequest.h>
#include <aws/dynamodb/model/DescribeTableResult.h>
#include <aws/dynamodb/model/DeleteItemRequest.h>
#include <aws/dynamodb/model/DeleteItemResult.h>
#include <aws/dynamodb/model/DeleteTableRequest.h>
#include <aws/dynamodb/model/DeleteTableResult.h>
#include <aws/dynamodb/model/GetItemRequest.h>
#include <aws/dynamodb/model/GetItemResult.h>
#include <aws/dynamodb/model/UpdateItemRequest.h>
#include <aws/dynamodb/model/UpdateItemResult.h>
#include <aws/dynamodb/model/TableDescription.h>
#include <aws/dynamodb/model/ScanRequest.h>
#include <aws/dynamodb/model/ScanResult.h>
#include <aws/dynamodb/model/PutItemRequest.h>
#include <aws/dynamodb/model/PutItemResult.h>
#include <aws/dynamodb/model/CreateTableRequest.h>
#include <aws/dynamodb/model/CreateTableResult.h>

namespace kclpp { namespace dynamo {

using client_ptr_t = KCLDynamoClient::client_ptr_t;
using aws_client_t = KCLDynamoClient::aws_client_t;
using state_ptr_t = KCLDynamoClient::state_ptr_t;
using KCLDynamoClientParams = KCLDynamoClient::KCLDynamoClientParams;
using KCLDynamoClientState = KCLDynamoClient::KCLDynamoClientState;
using CallbackTask = async::tpool::CallbackTask;


KCLDynamoClient::KCLDynamoClient(state_ptr_t&& state)
  : state_(std::move(state)){}

KCLDynamoClient* KCLDynamoClient::createNew(KCLDynamoClientParams&& params) {
  auto state = std::make_unique<KCLDynamoClientState>();
  state->params = std::move(params);
  state->client.reset(new aws_client_t{state->params.awsConfig});
  return new KCLDynamoClient(std::move(state));
}

void KCLDynamoClient::getItemAsync(GetItemRequest&& request, get_item_cb_t&& callback) {
  GetItemRequest realRequest {std::forward<GetItemRequest>(request)};
  get_item_cb_t realCb {std::forward<get_item_cb_t>(callback)};
  state_->params.asyncContext->runInEventThread(
    [this, callbackTemp = std::move(realCb), realRequest = std::move(realRequest)] {
      auto outcomePtr = std::make_shared<get_item_outcome_t>();
      state_->params.asyncContext->getThreadPool()->trySubmit(
        CallbackTask::createFromEventThread(
          state_->params.asyncContext->getEventContext(),
          [this, realRequest = std::move(realRequest), outcomePtr]() {
            GetItemRequest movedRequest = std::move(realRequest);
            auto outcome = state_->client->GetItem(movedRequest);
            *outcomePtr = outcome;
            return CallbackTask::done_outcome_t {Unit{}};          
          },
          [this, callback = std::move(callbackTemp), outcomePtr](const auto&) {
            callback(std::move(*outcomePtr));
          }
        )
      );
    }
  );
}

void KCLDynamoClient::getItemAsync(const GetItemRequest& request, get_item_cb_t&& callback) {
  GetItemRequest reqCopy = request;
  getItemAsync(std::move(reqCopy), std::forward<get_item_cb_t>(callback));
}

void KCLDynamoClient::putItemAsync(const PutItemRequest& request, put_item_cb_t callback) {
  PutItemRequest reqCopy = request;
  state_->params.asyncContext->runInEventThread([this, callback, reqCopy] {
    auto outcomePtr = std::make_shared<put_item_outcome_t>();
    state_->params.asyncContext->getThreadPool()->trySubmit(
      CallbackTask::createFromEventThread(
        state_->params.asyncContext->getEventContext(),
        [this, reqCopy, outcomePtr]() {
          auto outcome = state_->client->PutItem(reqCopy);
          *outcomePtr = outcome;
          return CallbackTask::done_outcome_t {Unit{}};          
        },
        [this, callback, outcomePtr](const auto&) {
          callback(*outcomePtr);
        }
      )
    );
  });
}

void KCLDynamoClient::updateItemAsync(const UpdateItemRequest& request, update_item_cb_t callback) {
  UpdateItemRequest reqCopy = request;
  state_->params.asyncContext->runInEventThread([this, callback, reqCopy] {
    auto outcomePtr = std::make_shared<update_item_outcome_t>();
    state_->params.asyncContext->getThreadPool()->trySubmit(
      CallbackTask::createFromEventThread(
        state_->params.asyncContext->getEventContext(),
        [this, reqCopy, outcomePtr]() {
          auto outcome = state_->client->UpdateItem(reqCopy);
          *outcomePtr = outcome;
          return CallbackTask::done_outcome_t {Unit{}};          
        },
        [this, callback, outcomePtr](const auto&) {
          callback(*outcomePtr);
        }
      )
    );
  }); 
}


void KCLDynamoClient::deleteItemAsync(const DeleteItemRequest& request, delete_item_cb_t callback) {
  DeleteItemRequest reqCopy = request;
  state_->params.asyncContext->runInEventThread([this, callback, reqCopy] {
    auto outcomePtr = std::make_shared<delete_item_outcome_t>();
    state_->params.asyncContext->getThreadPool()->trySubmit(
      CallbackTask::createFromEventThread(
        state_->params.asyncContext->getEventContext(),
        [this, reqCopy, outcomePtr]() {
          auto outcome = state_->client->DeleteItem(reqCopy);
          *outcomePtr = outcome;
          return CallbackTask::done_outcome_t {Unit{}};
        },
        [this, callback, outcomePtr](const auto&) {
          callback(*outcomePtr);
        }
      )
    );
  }); 
}


void KCLDynamoClient::scanAsync(const ScanRequest& request, scan_cb_t callback) {
  ScanRequest reqCopy = request;
  state_->params.asyncContext->runInEventThread([this, callback, reqCopy] {
    auto outcomePtr = std::make_shared<scan_outcome_t>();
    state_->params.asyncContext->getThreadPool()->trySubmit(
      CallbackTask::createFromEventThread(
        state_->params.asyncContext->getEventContext(),
        [this, reqCopy, outcomePtr]() {
          auto outcome = state_->client->Scan(reqCopy);
          *outcomePtr = outcome;
          return CallbackTask::done_outcome_t {Unit{}};          
        },
        [this, callback, outcomePtr](const auto&) {
          callback(*outcomePtr);
        }
      )
    );
  });
}


void KCLDynamoClient::createTableAsync(const CreateTableRequest& request, create_table_cb_t callback) {
  CreateTableRequest reqCopy = request;
  state_->params.asyncContext->runInEventThread([this, callback, reqCopy] {
    auto outcomePtr = std::make_shared<create_table_outcome_t>();
    state_->params.asyncContext->getThreadPool()->trySubmit(
      CallbackTask::createFromEventThread(
        state_->params.asyncContext->getEventContext(),
        [this, reqCopy, outcomePtr]() {
          auto outcome = state_->client->CreateTable(reqCopy);
          *outcomePtr = outcome;
          return CallbackTask::done_outcome_t {Unit{}};
        },
        [this, callback, outcomePtr](const auto&) {
          callback(*outcomePtr);
        }
      )
    );
  });
}


void KCLDynamoClient::listTablesAsync(const ListTablesRequest& request, list_tables_cb_t callback) {
  ListTablesRequest reqCopy = request;
  state_->params.asyncContext->runInEventThread([this, callback, reqCopy] {
    auto outcomePtr = std::make_shared<list_tables_outcome_t>();
    state_->params.asyncContext->getThreadPool()->trySubmit(
      CallbackTask::createFromEventThread(
        state_->params.asyncContext->getEventContext(),
        [this, reqCopy, outcomePtr]() {
          auto outcome = state_->client->ListTables(reqCopy);
          *outcomePtr = outcome;
          return CallbackTask::done_outcome_t {Unit{}};          
        },
        [this, callback, outcomePtr](const auto&) {
          callback(*outcomePtr);
        }
      )
    );
  });
}


void KCLDynamoClient::deleteTableAsync(const DeleteTableRequest& request, delete_table_cb_t callback) {
  DeleteTableRequest reqCopy = request;
  state_->params.asyncContext->runInEventThread([this, callback, reqCopy] {
    auto outcomePtr = std::make_shared<delete_table_outcome_t>();
    state_->params.asyncContext->getThreadPool()->trySubmit(
      CallbackTask::createFromEventThread(
        state_->params.asyncContext->getEventContext(),
        [this, reqCopy, outcomePtr]() {
          auto outcome = state_->client->DeleteTable(reqCopy);
          *outcomePtr = outcome;
        },
        [this, callback, outcomePtr](const auto&){
          callback(*outcomePtr);
        }
      )
    );
  });
}


void KCLDynamoClient::describeTableAsync(const DescribeTableRequest& request, describe_table_cb_t callback) {
  DescribeTableRequest reqCopy = request;
  state_->params.asyncContext->runInEventThread([this, callback, reqCopy] {
    auto outcomePtr = std::make_shared<describe_table_outcome_t>();
    state_->params.asyncContext->getThreadPool()->trySubmit(
      CallbackTask::createFromEventThread(
        state_->params.asyncContext->getEventContext(),
        [this, reqCopy, outcomePtr]() {
          auto outcome = state_->client->DescribeTable(reqCopy);
          *outcomePtr = outcome;
          return CallbackTask::done_outcome_t {Unit{}};          
        },
        [this, callback, outcomePtr](const auto&) {
          callback(*outcomePtr);
        }
      )
    );
  });
}


void KCLDynamoClient::doesTableExistAsync(const Aws::String& tableName, does_table_exist_cb_t callback) {
  auto request = DescribeTableRequest().WithTableName(tableName);
  describeTableAsync(request, [this, callback](const auto& outcome) {
    if (outcome.IsSuccess()) {
      DoesTableExistResult result;
      result.exists = true;
      callback(does_table_exist_outcome_t{result});
      return;
    }
    auto responseErr = outcome.GetError();
    if (responseErr.GetErrorType() == DynamoDBErrors::RESOURCE_NOT_FOUND) {
      DoesTableExistResult result;
      result.exists = false;
      callback(does_table_exist_outcome_t{result});
      return;
    }
    callback(does_table_exist_outcome_t{responseErr});
  });
}


}} // kclpp::dynamo
