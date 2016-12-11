#include <string>
#include <gtest/gtest.h>
#include <aws/dynamodb/model/AttributeValue.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include "kclpp/leases/Lease.h"

using namespace std;
using kclpp::leases::Lease;
using kclpp::leases::LeaseKey;
using kclpp::leases::LeaseOwner;
using kclpp::leases::LeaseCounter;
using kclpp::leases::CheckpointID;
using kclpp::leases::SubCheckpointID;
using kclpp::leases::NumberOfLeaseSwitches;

using Aws::DynamoDB::Model::AttributeValue;
using attr_map_t = Aws::Map<Aws::String, AttributeValue>;


TEST(TestLease, TestDeserialization) {
  attr_map_t attrs {
    {"leaseKey", AttributeValue{"the-key"}},
    {"leaseOwner", AttributeValue{"the-owner"}},
    {"leaseCounter", AttributeValue().SetN("500")},
    {"checkpoint", AttributeValue{"1234"}},
    {"checkpointSubSequenceNumber", AttributeValue{"0"}},
    {"ownerSwitchesSinceCheckpoint", AttributeValue().SetN("3")}
  };
  auto outcome = Lease::fromDynamoDict(attrs);
  EXPECT_TRUE(outcome.IsSuccess());
  auto result = outcome.GetResultWithOwnership();
  EXPECT_EQ(string{"the-key"}, result.getState().leaseKey.value());
  EXPECT_EQ(string{"the-owner"}, result.getState().leaseOwner.value().value());
  EXPECT_EQ(string{"1234"}, result.getState().checkpointID.value().value());
  EXPECT_EQ(string{"0"}, result.getState().subCheckpointID.value().value());
  EXPECT_TRUE(result.getState().leaseCounter.hasValue());
  EXPECT_EQ(500, result.getState().leaseCounter.value().value());
}

TEST(TestLease, TestSerialization) {
  Lease::LeaseState leaseState;
  leaseState.leaseKey = LeaseKey {"key-1"};
  leaseState.leaseOwner.assign( LeaseOwner{ "key-owner" } );
  leaseState.checkpointID.assign( CheckpointID{ "checkpoint-id" });
  leaseState.subCheckpointID.assign(
    SubCheckpointID{"checkpoint-sub-id"}
  );
  leaseState.ownerSwitchesSinceCheckpoint.assign(
    NumberOfLeaseSwitches{7}
  );
  leaseState.leaseCounter.assign(LeaseCounter{403});
  Lease lease {std::move(leaseState)};
  auto asDynamoDict = lease.toDynamoDict();

  EXPECT_EQ("key-1", asDynamoDict.find("leaseKey")->second.GetS());
  EXPECT_EQ("key-owner", asDynamoDict.find("leaseOwner")->second.GetS());
  EXPECT_EQ("403", asDynamoDict.find("leaseCounter")->second.GetN());
  EXPECT_EQ("7", asDynamoDict.find("ownerSwitchesSinceCheckpoint")->second.GetN());
  EXPECT_EQ("checkpoint-id", asDynamoDict.find("checkpoint")->second.GetS());
  EXPECT_EQ(
    "checkpoint-sub-id",
    asDynamoDict.find("checkpointSubSequenceNumber")->second.GetS()
  );
}


