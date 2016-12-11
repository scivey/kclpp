#include <gtest/gtest.h>
#include <aws/dynamodb/DynamoDBClient.h>
#include <aws/dynamodb/model/ListTablesRequest.h>
#include <aws/dynamodb/model/ListTablesResult.h>
#include "kclpp/clientlib/worker/Worker.h"
#include "kclpp/clientlib/record_processor/CallbackRecordProcessorFactory.h"
#include "kclpp/clientlib/record_processor/RecordProcessor.h"

#include "kclpp/clock/Clock.h"

#include "kclpp_test_support/TestRetryStrategy.h"
#include "kclpp_test_support/LeaseManagerTestContext.h"
#include "kclpp_test_support/LeaseCoordinatorTestContext.h"
#include "kclpp_test_support/MockClock.h"

#include "kclpp_test_support/misc.h"
#include "kclpp_test_support/TableGuard.h"

using namespace std;
using namespace kclpp::dynamo;
using namespace kclpp;
using namespace kclpp::leases;
using kclpp::locks::ThreadBaton;
using kclpp::clock::NanosecondPoint;
using kclpp::clock::NanosecondDuration;
using kclpp::clock::Clock;

using Aws::Client::ClientConfiguration;
using Aws::DynamoDB::Model::ListTablesRequest;
using Aws::DynamoDB::Model::GetItemRequest;
using Aws::DynamoDB::Model::DeleteTableRequest;

using namespace kclpp::clientlib::worker;
using namespace kclpp::clientlib::record_processor;

using kclpp_test_support::LeaseManagerTestContext;
using kclpp_test_support::LeaseCoordinatorTestContext;
using kclpp_test_support::makePostGuard;
using kclpp_test_support::MockClock;
using kclpp_test_support::TableGuard;


class AProcessor: public RecordProcessor {
 public:
  using unit_outcome_t = RecordProcessor::unit_outcome_t;
  using unit_cb_t = RecordProcessor::unit_cb_t;
  void initialize(InitializeInput&& input, unit_cb_t callback) override {
    callback(unit_outcome_t{});
  }
  void processRecords(ProcessRecordsInput&& input, unit_cb_t callback) override {
    callback(unit_outcome_t{});
  }
  void shutdown(ShutdownInput&& input, unit_cb_t callback) override {
    callback(unit_outcome_t{});
  }
};


struct WorkerTestContext {
  std::shared_ptr<LeaseCoordinatorTestContext> coordinatorCtx {nullptr};
  MockClock clock;
  std::shared_ptr<Worker> worker {nullptr};
  WorkerTestContext() {
    coordinatorCtx.reset(new LeaseCoordinatorTestContext);
    WorkerParams params;
    params.options.applicationName = "something";
    params.clock = std::shared_ptr<Clock>{
      &clock, [](auto){}
    };
    params.leaseCoordinator = coordinatorCtx->coordinator;
    params.asyncContext = coordinatorCtx->getAsyncContext();
    params.recordProcessorFactory = std::make_shared<CallbackRecordProcessorFactory>(
      []() {
        return std::make_shared<AProcessor>();
      }
    );
    worker = util::createShared<Worker>(std::move(params));
  }
  ~WorkerTestContext() {
    worker.reset();
    coordinatorCtx.reset();
  }
};

TEST(TestWorker, TestConstruction) {
  WorkerTestContext context;
  EXPECT_EQ(
    string{"something"},
    context.worker->dangerouslyGetParams()->options.applicationName
  );
}

