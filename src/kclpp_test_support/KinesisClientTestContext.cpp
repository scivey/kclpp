#include "kclpp_test_support/KinesisClientTestContext.h"
#include <glog/logging.h>
#include <gtest/gtest.h>

#include <aws/kinesis/model/StreamStatus.h>
#include <aws/kinesis/KinesisErrors.h>

#include "kclpp_test_support/testConfig.h"
#include "kclpp_test_support/misc.h"
#include "kclpp/async/KCLAsyncContext.h"
#include "kclpp/kinesis/KCLKinesisClient.h"
#include "kclpp/util/misc.h"


using namespace std;
using kclpp::kinesis::KCLKinesisClient;
using kclpp::async::KCLAsyncContext;
using kclpp::async::KCLAsyncContext;
using kclpp::locks::ThreadBaton;
using Aws::Kinesis::Model::StreamStatus;
using Aws::Kinesis::KinesisErrors;


namespace util = kclpp::util;

namespace kclpp_test_support {

KinesisClientTestContext::KinesisClientTestContext()
  : KinesisClientTestContext(util::createShared<KCLAsyncContext>()) {}

KinesisClientTestContext::KinesisClientTestContext(shared_ptr<KCLAsyncContext> asyncCtx) {
  asyncContext = asyncCtx;
  asyncContext->getThreadPool()->start();
  KCLKinesisClient::KCLKinesisClientParams params;
  params.asyncContext = asyncContext;
  params.awsConfig = makeClientConfig();
  client = util::createShared<KCLKinesisClient>(std::move(params));
}

KinesisClientTestContext::~KinesisClientTestContext() {
  client.reset();
  asyncContext.reset();
}

void KinesisClientTestContext::loopOnBaton(kclpp::locks::ThreadBaton& baton) {
  kclpp_test_support::loopOnBaton(asyncContext.get(), baton);
}

void KinesisClientTestContext::waitUntilStreamIsActive(const string& streamName) {
  bool isActive {false};
  while (!isActive) {
    ThreadBaton bat;
    client->describeStreamAsync(streamName,
      [&bat, &isActive](const auto& outcome) {
        auto guard = makePostGuard(bat);
        if (!outcome.IsSuccess()) {
          LOG(INFO) << "describeStreamAsync failed: " << outcome.GetError().GetMessage();
          isActive = true;
        } else {
          EXPECT_TRUE(outcome.IsSuccess());
          auto desc = outcome.GetResult().GetStreamDescription();
          auto status = desc.GetStreamStatus();
          if (status == StreamStatus::ACTIVE) {
            isActive = true;
          }
        }
      }
    );
    loopOnBaton(bat);
    if (!isActive) {
      std::this_thread::sleep_for(chrono::milliseconds{5});
    }
  }
}

void KinesisClientTestContext::waitUntilStreamDoesNotExist(const string& streamName) {
  bool exists {true};
  while (exists) {
    ThreadBaton bat;
    client->describeStreamAsync(streamName,
      [&bat, &exists](const auto& outcome) {
        auto guard = makePostGuard(bat);
        if (outcome.IsSuccess()) {
          // still exists
          ;
        } else {
          auto err = outcome.GetError();
          CHECK(err.GetErrorType() == KinesisErrors::RESOURCE_NOT_FOUND);
          exists = false;
        }
      }
    );
    loopOnBaton(bat);
    if (exists) {
      std::this_thread::sleep_for(chrono::milliseconds(5));
    }
  }
}

} // kclpp_test_support
