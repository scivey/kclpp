#include "kclpp/leases/dynamo_helpers.h"
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

using attr_map_t = dynamo_helpers::attr_map_t;
using expected_map_t = dynamo_helpers::expected_map_t;
using update_map_t = dynamo_helpers::update_map_t;

attr_map_t dynamo_helpers::makeLeaseKey(const String& leaseKey) {
  return Aws::Map<Aws::String, AttributeValue> {
    {"leaseKey", AttributeValue(leaseKey)}
  };
}


attr_map_t dynamo_helpers::makeLeaseKey(const lease_ptr_t& lease) {
  return makeLeaseKey(lease->getState().leaseKey.value());
}


expected_map_t dynamo_helpers::makeCASExpectation(const lease_ptr_t& lease) {
  expected_map_t expected;
  const auto& leaseState = lease->getState();
  if (leaseState.leaseOwner.hasValue()) {
    expected.insert(std::make_pair(
      "leaseOwner",
      ExpectedAttributeValue().WithValue(
        AttributeValue{leaseState.leaseOwner.value().value()}
      )
    ));
  } else {
    expected.insert(std::make_pair(
      "leaseOwner", ExpectedAttributeValue().WithExists(false)
    ));
  }
  if (leaseState.leaseCounter.hasValue()) {
    auto counterVal = leaseState.leaseCounter.value().value();
    std::ostringstream oss;
    oss << counterVal;
    expected.insert(std::make_pair(
      "leaseCounter",
      ExpectedAttributeValue().WithValue(AttributeValue().SetN({ oss.str() }))
    ));
  } else {
    expected.insert(std::make_pair(
      "leaseCounter",
      ExpectedAttributeValue().WithExists(false)
    ));
  }
  return expected;
}


update_map_t dynamo_helpers::makeUpdateValues(const lease_ptr_t& lease,
    const UpdateLeaseParams& params) {
  update_map_t result;
  auto oldOwner = lease->getState().leaseOwner;
  if (params.setOwnerToNull) {
    if (oldOwner.hasValue()) {
      result.insert(std::make_pair(
        "leaseOwner",
        AttributeValueUpdate().WithAction(AttributeAction::DELETE_)
      ));
    }
  } else if (params.newOwner.hasValue()) {
    auto newOwner = params.newOwner;
    if (!oldOwner.hasValue() || oldOwner.value() != newOwner.value()) {
      result.insert(std::make_pair(
        "leaseOwner",
        AttributeValueUpdate().WithValue(AttributeValue{ newOwner.value().value() })
      ));
      size_t iNumSwitches = 0;
      if (lease->getState().ownerSwitchesSinceCheckpoint.hasValue()) {
        iNumSwitches = lease->getState().ownerSwitchesSinceCheckpoint.value().value();
      }
      iNumSwitches++;
      std::ostringstream oss;
      oss << iNumSwitches;
      result.insert(std::make_pair(
        "ownerSwitchesSinceCheckpoint",
        AttributeValueUpdate().WithValue(AttributeValue().SetN(oss.str()))
      ));
    }
  }
  {
    auto oldCounter = lease->getState().leaseCounter;
    size_t nextCounter = 0;
    if (oldCounter.hasValue()) {
      nextCounter = oldCounter.value().value();
    }
    nextCounter++;
    std::ostringstream oss;
    oss << nextCounter;
    result.insert(std::make_pair(
      "leaseCounter",
      AttributeValueUpdate().WithValue(AttributeValue().SetN(oss.str()))
    ));
  }
  return result;
}


}} // kclpp::leases
