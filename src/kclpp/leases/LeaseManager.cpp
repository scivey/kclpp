#include "kclpp/leases/LeaseManager.h"
#include <aws/dynamodb/model/AttributeValue.h>
#include <aws/dynamodb/model/AttributeAction.h>
#include <aws/dynamodb/model/AttributeValueUpdate.h>
#include <aws/dynamodb/model/AttributeDefinition.h>
#include <aws/dynamodb/model/CreateTableRequest.h>
#include <aws/dynamodb/model/CreateTableResult.h>
#include <aws/dynamodb/model/DeleteItemRequest.h>
#include <aws/dynamodb/model/DeleteItemResult.h>
#include <aws/dynamodb/model/ExpectedAttributeValue.h>
#include <aws/dynamodb/model/GetItemRequest.h>
#include <aws/dynamodb/model/GetItemResult.h>
#include <aws/dynamodb/model/KeySchemaElement.h>
#include <aws/dynamodb/model/KeyType.h>
#include <aws/dynamodb/model/PutItemRequest.h>
#include <aws/dynamodb/model/PutItemResult.h>
#include <aws/dynamodb/model/ProvisionedThroughput.h>
#include <aws/dynamodb/model/ScanRequest.h>
#include <aws/dynamodb/model/ScanResult.h>
#include <aws/dynamodb/model/UpdateItemRequest.h>
#include <aws/dynamodb/model/UpdateItemResult.h>

#include "kclpp/dynamo/KCLDynamoClient.h"
#include "kclpp/leases/Lease.h"
#include "kclpp/leases/types.h"
#include "kclpp/dynamo/types.h"
#include "kclpp/leases/dynamo_helpers.h"

using namespace Aws::DynamoDB::Model;

namespace kclpp { namespace leases {

using dynamo_client_t = LeaseManager::dynamo_client_t;
using dynamo_client_ptr_t = LeaseManager::dynamo_client_ptr_t;
using create_shared_outcome_t = LeaseManager::create_shared_outcome_t;
using state_ptr_t = LeaseManager::state_ptr_t;
using kclpp::dynamo::KCLDynamoClient;

LeaseManager::LeaseManager(state_ptr_t&& initialState)
  : state_(std::move(initialState)) {}

create_shared_outcome_t LeaseManager::createShared(LeaseManagerParams&& params) {
  if (params.tableName.value().empty()) {
    return create_shared_outcome_t {InvalidState{"Table name cannot be empty"}};
  }
  if (!params.dynamoClient) {
    return create_shared_outcome_t {InvalidState{"Dynamo client cannot be nullptr."}};
  }
  if (!params.asyncContext) {
    return create_shared_outcome_t {InvalidState{"Async context cannot be nullptr."}};    
  }
  auto mgrState = std::make_unique<LeaseManagerState>();
  mgrState->params = std::move(params);
  std::shared_ptr<LeaseManager> manager {
    new LeaseManager { std::move(mgrState) }
  };
  return create_shared_outcome_t {std::move(manager)};
}

void LeaseManager::createTableIfNotExists(const CreateTableParams& params,
    create_table_cb_t callback) {
  auto request = CreateTableRequest()
      .WithTableName(state_->params.tableName.value())
      .WithProvisionedThroughput(
        ProvisionedThroughput()
          .WithReadCapacityUnits(params.readCapacity)
          .WithWriteCapacityUnits(params.writeCapacity)
      );

  request.AddAttributeDefinitions(
    AttributeDefinition()
      .WithAttributeType(ScalarAttributeType::S)
      .WithAttributeName("leaseKey")
  );
  request.AddKeySchema(
    KeySchemaElement()
      .WithAttributeName("leaseKey")
      .WithKeyType(KeyType::HASH)
  );

  state_->params.dynamoClient->createTableAsync(request,
    [callback = std::move(callback)]
    (const CreateTableOutcome& outcome) {
      if (!outcome.IsSuccess()) {
        callback(create_table_outcome_t{outcome.GetError()});
        return;
      }
      CreateTableIfNotExistsResult result;
      result.created = true;
      callback(create_table_outcome_t{result});
    }
  );
}


void LeaseManager::listLeases(list_leases_cb_t callback) {
  ScanRequest request;
  request.SetTableName(state_->params.tableName.value());
  request.SetLimit(100);
  auto onFinish = [callback = std::move(callback)](const ScanOutcome& scanOutcome) {
    if (!scanOutcome.IsSuccess()) {
      callback(list_leases_outcome_t {scanOutcome.GetError()});
      return;
    }
    auto result = scanOutcome.GetResult();
    std::vector<lease_ptr_t> leases;
    leases.reserve(result.GetItems().size());
    for (const auto& item: result.GetItems()) {
      auto deserialized = Lease::fromDynamoDict(item);
      if (!deserialized.IsSuccess()) {
        LOG(INFO) << "deserialization error! '" << deserialized.GetError().what() << "'";
        continue;
      }
      leases.emplace_back(new Lease{deserialized.GetResult()});
    }
    callback(std::move(leases));
  };
  state_->params.dynamoClient->scanAsync(request, onFinish);
}


void LeaseManager::handleDeleteResponse(const DeleteItemOutcome& outcome,
    delete_lease_cb_t callback) {
  if (!outcome.IsSuccess()) {
    callback( {delete_lease_outcome_t{ outcome.GetError() }});
    return;
  }

  // does this contain anything?
  auto result = outcome.GetResult();
  (void) result; // unused for now
  DeleteLeaseResult toReturn;
  toReturn.deleted = true; // check this
  callback( {delete_lease_outcome_t{ toReturn }});
}

void LeaseManager::deleteLeaseCAS(lease_ptr_t lease, delete_lease_cb_t callback) {
  DeleteItemRequest request;
  request.SetTableName(state_->params.tableName.value());
  request.SetKey(
    dynamo_helpers::makeLeaseKey(lease)
  );
  request.SetExpected(
    dynamo_helpers::makeCASExpectation(lease)
  );
  state_->params.dynamoClient->deleteItemAsync(request,
    [this, callback = std::move(callback)]
    (const KCLDynamoClient::delete_item_outcome_t& outcome) {
      handleDeleteResponse(outcome, callback);
    }
  );
}

void LeaseManager::deleteLeaseUnconditionally(lease_ptr_t lease, delete_lease_cb_t callback) {
  DeleteItemRequest request;
  request.SetTableName(state_->params.tableName.value());
  request.SetKey(
    dynamo_helpers::makeLeaseKey(lease)
  );
  state_->params.dynamoClient->deleteItemAsync(request,
    [this, callback = std::move(callback)]
    (const KCLDynamoClient::delete_item_outcome_t& outcome) {
      handleDeleteResponse(outcome, callback);
    }
  );
}


// common error handling in initial callback layer
void LeaseManager::internalUpdateLease(const InternalUpdateLeaseParams& params,
      internal_update_cb_t callback) {
  DCHECK(!!params.lease);
  auto expectations = dynamo_helpers::makeCASExpectation(params.lease);
  auto request = UpdateItemRequest()
      .WithTableName(state_->params.tableName.value())
      .WithKey(dynamo_helpers::makeLeaseKey(params.lease))
      .WithExpected(dynamo_helpers::makeCASExpectation(params.lease))
      .WithAttributeUpdates(dynamo_helpers::makeUpdateValues(
        params.lease, params.updateParams
      ));
  state_->params.dynamoClient->updateItemAsync(request,
    [request, callback = std::move(callback), params]
    (const KCLDynamoClient::update_item_outcome_t& outcome) {
      if (!outcome.IsSuccess()) {
        LOG(INFO) << "internalUpdateLease error: "
                  << "'" << outcome.GetError().GetExceptionName() << "'"
                  << " : '" << outcome.GetError().GetMessage() << "'";
      }
      callback(outcome, request);
    }
  );
}


// internalUpdateLease doesn't quite match what the Java KCL does for `updateLease`.
// instead, keeping duplication for now.
void LeaseManager::updateLease(lease_ptr_t lease, update_lease_cb_t callback) {
  UpdateItemRequest request;
  auto toUpdate = dynamo_helpers::makeUpdateValues(lease);
  const auto& leaseState = lease->getState();
  if (leaseState.checkpointID.hasValue()) {
    toUpdate.insert(std::make_pair(
      "checkpoint",
      AttributeValueUpdate().WithValue(
        AttributeValue{leaseState.checkpointID.value().value()}
      )
    ));
  }
  if (leaseState.subCheckpointID.hasValue()) {
    toUpdate.insert(std::make_pair(
      "checkpointSubSequenceNumber",
      AttributeValueUpdate().WithValue(
        AttributeValue{leaseState.subCheckpointID.value().value()}
      )
    ));
  }
  request
    .WithKey(dynamo_helpers::makeLeaseKey(lease))
    .WithExpected(dynamo_helpers::makeCASExpectation(lease))
    .WithAttributeUpdates(toUpdate);
  state_->params.dynamoClient->updateItemAsync(request,
    [callback = std::move(callback), lease, request]
    (const KCLDynamoClient::update_item_outcome_t& outcome) {
      if (!outcome.IsSuccess()) {
        callback(update_lease_outcome_t{outcome.GetError()});
        return;
      }
      Lease::LeaseState newState = lease->getState();
      UpdateLeaseResult result;
      result.lease = lease;
      result.updated = true;
      callback(update_lease_outcome_t{std::move(result)});
    }
  );
}

void LeaseManager::isLeaseTableEmpty(is_table_empty_cb_t callback) {
  listLeases([callback = std::move(callback)](const list_leases_outcome_t& outcome) {
    if (!outcome.IsSuccess()) {
      callback(is_table_empty_outcome_t{outcome.GetError()});
      return;
    }
    IsLeaseTableEmptyResult result;
    auto leases = outcome.GetResult();
    result.isEmpty = leases.empty();
    callback(result);
  });
}

void LeaseManager::createLeaseIfNotExists(lease_ptr_t lease, create_lease_cb_t callback) {
  PutItemRequest request;
  request.SetTableName(state_->params.tableName.value());
  request.SetItem(lease->toDynamoDict());
  Aws::Map<Aws::String, ExpectedAttributeValue> expected;
  ExpectedAttributeValue nonExistent;
  nonExistent.SetExists(false);
  expected.insert(std::make_pair(
    "leaseKey", nonExistent
  ));
  request.SetExpected(std::move(expected));
  state_->params.dynamoClient->putItemAsync(request,
    [lease, callback = std::move(callback)]
    (const PutItemOutcome& outcome) {
      if (!outcome.IsSuccess()) {
        if (outcome.GetError().GetErrorType() == DynamoDBErrors::CONDITIONAL_CHECK_FAILED) {
          // the request went through, but the lease already existed.
          CreateLeaseIfNotExistsResult result;
          result.created = false;
          callback(create_lease_outcome_t {result});
          return;
        }
        callback(create_lease_outcome_t {outcome.GetError()});
        return;
      }
      CreateLeaseIfNotExistsResult result;
      result.created = true;
      result.lease.assign(lease);
      callback(create_lease_outcome_t {std::move(result)});
    }
  );
}


// this may need a wrapping error type: does aws_errors_t include a failed GetItemRequest?
void LeaseManager::getLease(LeaseKey key, get_lease_cb_t callback) {
  GetItemRequest request;
  request.SetTableName(state_->params.tableName.value());
  Aws::Map<Aws::String, AttributeValue> keyVals {
    {"leaseKey", AttributeValue{key.value()}}
  };
  request.SetKey(keyVals);
  state_->params.dynamoClient->getItemAsync(std::move(request),
    [key, callback = std::move(callback)]
    (const GetItemOutcome& outcome) {
      if (!outcome.IsSuccess()) {
        callback(get_lease_outcome_t{ outcome.GetError()});
        return;
      }
      auto parsedLease = Lease::fromDynamoDict(outcome.GetResult().GetItem());
      CHECK(parsedLease.IsSuccess());
      GetLeaseResult gottenLease;
      gottenLease.lease.reset(new Lease{parsedLease.GetResultWithOwnership()});
      callback(get_lease_outcome_t{std::move(gottenLease)});
    }
  );
}

void LeaseManager::renewLease(lease_ptr_t lease, renew_lease_cb_t callback) {
  InternalUpdateLeaseParams internalParams;
  internalParams.lease = lease;
  internalUpdateLease(internalParams,
    [callback = std::move(callback), lease]
    (const internal_update_outcome_t& outcome, const UpdateItemRequest& request) {
      if (!outcome.IsSuccess()) {
        callback(renew_lease_outcome_t{outcome.GetError()});
        return;
      }
      RenewLeaseResult result;
      result.renewed = true;
      result.lease = lease->copySharedWithIncrementedCounter();
      callback(renew_lease_outcome_t{std::move(result)});
    }
  );
}

using evict_lease_outcome_t = LeaseManager::evict_lease_outcome_t;
using evict_lease_cb_t = LeaseManager::evict_lease_cb_t;
void LeaseManager::evictLease(lease_ptr_t lease, evict_lease_cb_t callback) {
  InternalUpdateLeaseParams internalParams;
  internalParams.lease = lease;
  internalParams.updateParams.setOwnerToNull = true;
  internalUpdateLease(internalParams,
    [callback = std::move(callback)]
    (const internal_update_outcome_t& outcome, const UpdateItemRequest& request) {
      if (!outcome.IsSuccess()) {
        callback(evict_lease_outcome_t { outcome.GetError() });
        return;
      }
      EvictLeaseResult finalResult;
      finalResult.evicted = true;
      callback(evict_lease_outcome_t{ finalResult });
    }
  );
}


void LeaseManager::takeLease(lease_ptr_t lease, LeaseOwner newOwner, take_lease_cb_t callback) {
  InternalUpdateLeaseParams internalParams;
  internalParams.lease = lease;
  internalParams.updateParams.newOwner.assign(newOwner);
  internalUpdateLease(internalParams,
    [callback = std::move(callback), lease]
    (const internal_update_outcome_t& outcome, const UpdateItemRequest& request) {
      if (!outcome.IsSuccess()) {
        callback(take_lease_outcome_t{outcome.GetError()});
        return;
      }
      TakeLeaseResult result;
      result.lease = lease;
      result.taken = true;
      callback(take_lease_outcome_t{result});
    }
  );
}

void LeaseManager::waitUntilLeaseTableExists(const WaitUntilTableExistsParams& params, 
    wait_until_exists_cb_t callback) {
  state_->params.dynamoClient->doesTableExistAsync(state_->params.tableName.value(),
    [this, callback = std::move(callback)](const auto& outcome) {
      callback(outcome);
    }
  );
}


}} // kclpp::leases
