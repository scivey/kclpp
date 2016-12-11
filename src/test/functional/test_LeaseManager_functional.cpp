#include <gtest/gtest.h>
#include <aws/dynamodb/DynamoDBClient.h>
#include <aws/dynamodb/model/ListTablesRequest.h>
#include <aws/dynamodb/model/ListTablesResult.h>
#include <aws/dynamodb/model/DeleteTableRequest.h>
#include <aws/dynamodb/model/GetItemRequest.h>
#include <aws/dynamodb/model/GetItemResult.h>

#include "kclpp/leases/LeaseManager.h"
#include "kclpp/leases/Lease.h"
#include "kclpp/dynamo/KCLDynamoClient.h"
#include "kclpp/APIGuard.h"
#include "kclpp/ScopeGuard.h"
#include "kclpp/UniqueToken.h"
#include "kclpp/locks/ThreadBaton.h"
#include "kclpp/TimeLogger.h"
#include "kclpp_test_support/TestRetryStrategy.h"
#include "kclpp_test_support/LeaseManagerTestContext.h"
#include "kclpp_test_support/misc.h"
#include "kclpp_test_support/TableGuard.h"

using namespace std;
using namespace kclpp::dynamo;
using namespace kclpp;
using namespace kclpp::leases;
using kclpp::locks::ThreadBaton;
using kclpp::TimeLogger;
using Aws::Client::ClientConfiguration;
using Aws::DynamoDB::Model::ListTablesRequest;
using Aws::DynamoDB::Model::GetItemRequest;
using Aws::DynamoDB::Model::DeleteTableRequest;

using kclpp_test_support::LeaseManagerTestContext;
using kclpp_test_support::makePostGuard;
using kclpp_test_support::TableGuard;
using kclpp_test_support::buildSimpleLease;


TEST(TestLeaseManager, TestCreateTableIfNotExistsHappy) {
  TimeLogger tl = TimeLogger::create("foo");
  {
    auto tinner1 = TimeLogger::create("inner1");
    LeaseManagerTestContext context;
    {
      auto tinner2 = TimeLogger::create("inner2");
      auto client = context.client;
      auto manager = context.manager;
      const std::string kTableName= context.tableName;
      {
        auto t1 = TimeLogger::create("bat1");
        ThreadBaton bat1;
        client->doesTableExistAsync(kTableName,
          [&bat1](const KCLDynamoClient::does_table_exist_outcome_t& outcome) {
            auto guard = makePostGuard(bat1);
            EXPECT_TRUE(outcome.IsSuccess());
            EXPECT_FALSE(outcome.GetResult().exists);
          }
        );
        context.loopOnBaton(bat1);
      }
      {
        auto t2 = TimeLogger::create("bat2");
        ThreadBaton bat2;
        manager->createTableIfNotExists(LeaseManager::CreateTableParams(),
          [&bat2](const LeaseManager::create_table_outcome_t& outcome) {
            auto guard = makePostGuard(bat2);
            EXPECT_TRUE(outcome.IsSuccess());
            EXPECT_TRUE(outcome.GetResult().created);
          }
        );
        context.loopOnBaton(bat2);
      }
      {
        auto t3 = TimeLogger::create("bat3");
        ThreadBaton bat3;
        client->doesTableExistAsync(kTableName,
          [&bat3](const KCLDynamoClient::does_table_exist_outcome_t& outcome) {
            auto guard = makePostGuard(bat3);
            EXPECT_TRUE(outcome.IsSuccess());
            EXPECT_TRUE(outcome.GetResult().exists);
          }
        );
        context.loopOnBaton(bat3);
      }
      {
        auto t4 = TimeLogger::create("bat4");
        ThreadBaton bat4;
        client->deleteTableAsync(DeleteTableRequest().WithTableName(kTableName),
          [&bat4](const KCLDynamoClient::delete_table_outcome_t& outcome) {
            auto guard = makePostGuard(bat4);
            EXPECT_TRUE(outcome.IsSuccess());
          }
        );
        context.loopOnBaton(bat4);
        
      }
    }
  }
}



TEST(TestLeaseManager, TestTableGuard) {
  LeaseManagerTestContext context;
  auto client = context.client;
  auto manager = context.manager;
  const std::string kTableName= context.tableName;
  {
    auto tableGuard = TableGuard::create(&context);
    {
      ThreadBaton bat1;
      client->doesTableExistAsync(kTableName,
        [&bat1](const KCLDynamoClient::does_table_exist_outcome_t& outcome) {
          auto guard = makePostGuard(bat1);
          EXPECT_TRUE(outcome.IsSuccess());
          EXPECT_TRUE(outcome.GetResult().exists);
        }
      );
      context.loopOnBaton(bat1);
    }
  }
  {
    ThreadBaton bat;
    client->doesTableExistAsync(kTableName,
      [&bat](const KCLDynamoClient::does_table_exist_outcome_t& outcome) {
        auto guard = makePostGuard(bat);
        EXPECT_TRUE(outcome.IsSuccess());
        EXPECT_FALSE(outcome.GetResult().exists);
      }
    );
    context.loopOnBaton(bat);
  }  
}

TEST(TestLeaseManager, TestCreateTableIfNotExistsSadPanda) {
  LeaseManagerTestContext context;
  auto client = context.client;
  auto manager = context.manager;
  const std::string kTableName= context.tableName;
  {
    auto tableGuard = TableGuard::create(&context);
    {
      ThreadBaton bat;
      manager->createTableIfNotExists(LeaseManager::CreateTableParams(),
        [&bat](const LeaseManager::create_table_outcome_t& outcome) {
          auto guard = makePostGuard(bat);
          EXPECT_FALSE(outcome.IsSuccess());
        }
      );
      context.loopOnBaton(bat);
    }
  }
}




TEST(TestLeaseManager, TestCreateLeaseIfNotExistsHappy) {
  bool consistentReads = true;
  LeaseManagerTestContext context {consistentReads};
  auto client = context.client;
  auto manager = context.manager;
  const std::string kTableName = context.tableName;
  {
    auto tableGuard = TableGuard::create(&context);
    {
      Lease::LeaseState leaseState;
      leaseState.leaseKey = LeaseKey {"shard-1-id"};
      leaseState.leaseOwner.assign(LeaseOwner{"shard-1-owner"});
      auto lease = std::make_shared<Lease>(std::move(leaseState));
      ThreadBaton bat2;
      manager->createLeaseIfNotExists(lease,
        [&bat2](const LeaseManager::create_lease_outcome_t& outcome) {
          auto guard = makePostGuard(bat2);
          EXPECT_TRUE(outcome.IsSuccess())
            << "failure: '" << outcome.GetError().GetMessage() << "'";
        }
      );
      context.loopOnBaton(bat2);
    }
    {
      context.pollUntilAnyLeaseExists();
      ThreadBaton bat3;
      manager->listLeases([&bat3](const LeaseManager::list_leases_outcome_t& outcome) {
        auto guard = makePostGuard(bat3);
        EXPECT_EQ(1, outcome.GetResult().size());
        auto lease = outcome.GetResult().at(0);
        EXPECT_EQ("shard-1-id", lease->getState().leaseKey.value());
      });
      context.loopOnBaton(bat3);
    }
  }
}


TEST(TestLeaseManager, TestCreateLeaseIfNotExistsSadPanda) {
  bool consistentReads = true;
  LeaseManagerTestContext context {consistentReads};
  auto client = context.client;
  auto manager = context.manager;
  const std::string kTableName = context.tableName;
  {
    auto tableGuard = TableGuard::create(&context);
    {
      Lease::LeaseState leaseState;
      leaseState.leaseKey = LeaseKey {"shard-1-id"};
      leaseState.leaseOwner.assign(LeaseOwner{"shard-1-owner"});
      auto lease = std::make_shared<Lease>(std::move(leaseState));
      ThreadBaton bat;
      manager->createLeaseIfNotExists(lease,
        [&bat](const LeaseManager::create_lease_outcome_t& outcome) {
          auto guard = makePostGuard(bat);
          EXPECT_TRUE(outcome.IsSuccess())
            << "failure: '" << outcome.GetError().GetMessage() << "'";
        }
      );
      context.loopOnBaton(bat);
    }
    context.pollUntilAnyLeaseExists();
    auto checkOwnerIsCorrect = [client, kTableName, &context]() {
      ThreadBaton bat;
      auto request = GetItemRequest()
          .WithKey(dynamo_helpers::makeLeaseKey("shard-1-id"))
          .WithTableName(kTableName);
      client->getItemAsync(std::move(request),
        [&bat](const KCLDynamoClient::get_item_outcome_t& outcome) {
          auto guard = makePostGuard(bat);
          EXPECT_TRUE(outcome.IsSuccess());
          auto deserialized = Lease::fromDynamoDict(outcome.GetResult().GetItem());
          EXPECT_TRUE(deserialized.IsSuccess());
          auto retrievedLease = deserialized.GetResultWithOwnership();
          auto leaseOwner = retrievedLease.getState().leaseOwner;
          EXPECT_TRUE(leaseOwner.hasValue());
          if (leaseOwner.hasValue()) {
            EXPECT_EQ(
              "shard-1-owner",
              leaseOwner.value().value()
            );
          }
        }
      );
      context.loopOnBaton(bat);
    };
    checkOwnerIsCorrect();
    {
      Lease::LeaseState leaseState;
      leaseState.leaseKey = LeaseKey {"shard-1-id"};
      leaseState.leaseOwner.assign(LeaseOwner{"bad-owner-id"});
      auto lease = std::make_shared<Lease>(std::move(leaseState));
      ThreadBaton bat3;
      manager->createLeaseIfNotExists(lease,
        [&bat3](const LeaseManager::create_lease_outcome_t& outcome) {
          auto guard = makePostGuard(bat3);
          EXPECT_TRUE(outcome.IsSuccess())
            << "Failure: '" << outcome.GetError().GetMessage() << "'";
          EXPECT_FALSE(outcome.GetResult().created);
        }
      );
      context.loopOnBaton(bat3);
    }
    checkOwnerIsCorrect();
  }
}


TEST(TestLeaseManager, TestDeleteLeaseCASHappy) {
  bool consistentReads = true;
  LeaseManagerTestContext context {consistentReads};
  auto client = context.client;
  auto manager = context.manager;
  const std::string kTableName = context.tableName;
  {
    auto tableGuard = TableGuard::create(&context);
    {
      auto lease = buildSimpleLease("shard-1-id", "shard-1-owner", 1);
      context.saveLeaseSync(lease);
    }
    context.pollUntilAnyLeaseExists();
    {
      ThreadBaton bat;
      auto lease = buildSimpleLease("shard-1-id", "shard-1-owner", 1);
      manager->deleteLeaseCAS(lease,
        [&bat](const LeaseManager::delete_lease_outcome_t& outcome) {
          auto guard = makePostGuard(bat);
          EXPECT_TRUE(outcome.IsSuccess())
            << "Failure: '" << outcome.GetError().GetMessage() << "'";
          EXPECT_TRUE(outcome.GetResult().deleted);
        }
      );
      context.loopOnBaton(bat);
    }
  }
}


TEST(TestLeaseManager, TestDeleteLeaseCASPandaIsWrongOwner) {
  bool consistentReads = true;
  LeaseManagerTestContext context {consistentReads};
  auto client = context.client;
  auto manager = context.manager;
  const std::string kTableName = context.tableName;
  {
    auto tableGuard = TableGuard::create(&context);
    {
      auto lease = buildSimpleLease("shard-1-id", "shard-1-owner", 1);
      context.saveLeaseSync(lease);
    }
    context.pollUntilAnyLeaseExists();
    {
      Lease::LeaseState leaseState;
      auto lease = buildSimpleLease("shard-1-id", "bad-owner", 1);
      ThreadBaton bat;
      manager->deleteLeaseCAS(lease,
        [&bat](const LeaseManager::delete_lease_outcome_t& outcome) {
          auto guard = makePostGuard(bat);
          EXPECT_FALSE(outcome.IsSuccess());
        }
      );
      context.loopOnBaton(bat);
    }
  }
}


TEST(TestLeaseManager, TestDeleteLeaseCASPandaHasWrongVersionBadPandaBad) {
  bool consistentReads = true;
  LeaseManagerTestContext context {consistentReads};
  auto client = context.client;
  auto manager = context.manager;
  const std::string kTableName = context.tableName;
  {
    auto tableGuard = TableGuard::create(&context);
    {
      auto lease = buildSimpleLease("shard-1-id", "shard-1-owner", 5);
      context.saveLeaseSync(lease);
    }
    context.pollUntilAnyLeaseExists();
    {
      auto lease = buildSimpleLease("shard-1-id", "shard-1-owner", 4);
      ThreadBaton bat;
      manager->deleteLeaseCAS(lease,
        [&bat](const LeaseManager::delete_lease_outcome_t& outcome) {
          auto guard = makePostGuard(bat);
          EXPECT_FALSE(outcome.IsSuccess());
        }
      );
      context.loopOnBaton(bat);
    }
  }
}


// the deletion is unconditional, not the happiness
TEST(TestLeaseManager, TestDeleteLeaseUnconditionallyHappy) {
  bool consistentReads = true;
  LeaseManagerTestContext context {consistentReads};
  auto client = context.client;
  auto manager = context.manager;
  const std::string kTableName = context.tableName;
  {
    auto tableGuard = TableGuard::create(&context);
    {
      auto lease = buildSimpleLease("shard-1-id", "shard-1-owner", 5);
      context.saveLeaseSync(lease);
    }
    context.pollUntilAnyLeaseExists();
    {
      auto lease = buildSimpleLease("shard-1-id", "shard-1-owner", 5);
      ThreadBaton bat;
      manager->deleteLeaseUnconditionally(lease,
        [&bat](const LeaseManager::delete_lease_outcome_t& outcome) {
          auto guard = makePostGuard(bat);
          EXPECT_TRUE(outcome.IsSuccess());
        }
      );
      context.loopOnBaton(bat);
    }
  }
}


TEST(TestLeaseManager, TestDeleteLeaseUnconditionallyStillRelativelyContent) {
  bool consistentReads = true;
  LeaseManagerTestContext context {consistentReads};
  auto client = context.client;
  auto manager = context.manager;
  const std::string kTableName = context.tableName;
  {
    auto tableGuard = TableGuard::create(&context);
    {
      auto lease = buildSimpleLease("shard-1-id", "shard-1-owner", 5);
      context.saveLeaseSync(lease);
    }
    context.pollUntilAnyLeaseExists();
    {
      auto lease = buildSimpleLease("shard-1-id", "shard-1-owner", 3);
      ThreadBaton bat;
      manager->deleteLeaseUnconditionally(lease,
        [&bat](const LeaseManager::delete_lease_outcome_t& outcome) {
          auto guard = makePostGuard(bat);
          EXPECT_TRUE(outcome.IsSuccess());
        }
      );
      context.loopOnBaton(bat);
    }
  }
}


TEST(TestLeaseManager, TestIsLeaseTableEmptyYes) {
  bool consistentReads = true;
  LeaseManagerTestContext context {consistentReads};
  auto client = context.client;
  auto manager = context.manager;
  const std::string kTableName = context.tableName;
  {
    auto tableGuard = TableGuard::create(&context);
    {
      ThreadBaton bat;
      manager->isLeaseTableEmpty(
        [&bat](const LeaseManager::is_table_empty_outcome_t& outcome) {
          auto guard = makePostGuard(bat);
          EXPECT_TRUE(outcome.IsSuccess());
          EXPECT_TRUE(outcome.GetResult().isEmpty);
        }
      );
      context.loopOnBaton(bat);
    }
  }
}

TEST(TestLeaseManager, TestIsLeaseTableEmptyNo) {
  bool consistentReads = true;
  LeaseManagerTestContext context {consistentReads};
  auto client = context.client;
  auto manager = context.manager;
  const std::string kTableName = context.tableName;
  {
    auto tableGuard = TableGuard::create(&context);
    {
      auto lease = buildSimpleLease("shard-1-id", "shard-1-owner", 1);
      context.saveLeaseSync(lease);
    }
    {
      ThreadBaton bat;
      manager->isLeaseTableEmpty(
        [&bat](const LeaseManager::is_table_empty_outcome_t& outcome) {
          auto guard = makePostGuard(bat);
          EXPECT_TRUE(outcome.IsSuccess());
          EXPECT_FALSE(outcome.GetResult().isEmpty);
        }
      );
      context.loopOnBaton(bat);
    }
  }
}
