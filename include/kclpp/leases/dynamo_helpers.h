#pragma once
#include <aws/dynamodb/model/ProvisionedThroughput.h>
#include <aws/dynamodb/model/KeySchemaElement.h>
#include <aws/dynamodb/model/AttributeValue.h>
#include <aws/dynamodb/model/AttributeAction.h>
#include <aws/dynamodb/model/AttributeValueUpdate.h>
#include <aws/dynamodb/model/AttributeDefinition.h>
#include <aws/dynamodb/model/ExpectedAttributeValue.h>

#include <aws/dynamodb/model/KeyType.h>


#include "kclpp/dynamo/KCLDynamoClient.h"
#include "kclpp/leases/Lease.h"
#include "kclpp/dynamo/types.h"

namespace kclpp { namespace leases {

struct dynamo_helpers {
  using AttributeValue = Aws::DynamoDB::Model::AttributeValue;
  using ExpectedAttributeValue = Aws::DynamoDB::Model::ExpectedAttributeValue;
  using AttributeValueUpdate = Aws::DynamoDB::Model::AttributeValueUpdate;
  using AttributeAction = Aws::DynamoDB::Model::AttributeAction;

  using String = Aws::String;
  using attr_map_t = Aws::Map<Aws::String, AttributeValue>;
  using expected_map_t = Aws::Map<Aws::String, ExpectedAttributeValue>;
  using update_map_t = Aws::Map<String, AttributeValueUpdate>;
  using lease_ptr_t = std::shared_ptr<Lease>;

  static attr_map_t makeLeaseKey(const String& leaseKey);
  static attr_map_t makeLeaseKey(const lease_ptr_t& lease);
  static expected_map_t makeCASExpectation(const lease_ptr_t& lease);

  struct UpdateLeaseParams {
    Optional<LeaseOwner> newOwner;
    bool setOwnerToNull {false};
  };
  static update_map_t makeUpdateValues(const lease_ptr_t& lease,
    const UpdateLeaseParams& params = UpdateLeaseParams()
  );
};


}}