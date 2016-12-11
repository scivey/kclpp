#include "kclpp_test_support/StreamGuard.h"
#include "kclpp_test_support/misc.h"
#include "kclpp/UniqueToken.h"
#include <gtest/gtest.h>
#include <string>
#include <aws/kinesis/model/DeleteStreamRequest.h>
#include <aws/kinesis/model/CreateStreamRequest.h>

using Aws::Kinesis::Model::DeleteStreamRequest;
using Aws::Kinesis::Model::CreateStreamRequest;

using namespace std;
using kclpp::UniqueToken;
using kclpp::locks::ThreadBaton;

namespace kclpp_test_support {

StreamGuardOptions& StreamGuardOptions::withName(const string& name) {
  streamName = name;
  return *this;
}
StreamGuardOptions& StreamGuardOptions::withNumShards(size_t n) {
  numShards = n;
  return *this;
}

StreamGuard::StreamGuard(UniqueToken&& token, KinesisClientTestContext* ctx, const StreamGuardOptions& options)
  : token_(std::move(token)),
    context_(ctx),
    options_(options) {}


StreamGuard::StreamGuard(StreamGuard&& other): token_(std::move(other.token_)) {
  context_ = other.context_;
  options_ = other.options_;
}

StreamGuard& StreamGuard::operator=(StreamGuard&& other) {
  std::swap(token_, other.token_);
  std::swap(context_, other.context_);
  std::swap(options_, other.options_);
  return *this;
}

StreamGuard StreamGuard::create(KinesisClientTestContext *ctx, const StreamGuardOptions& options) {
  kclpp::locks::ThreadBaton bat;
  auto request = CreateStreamRequest()
    .WithStreamName(options.streamName)
    .WithShardCount(options.numShards);
  ctx->client->createStreamAsync(request, [&bat, ctx](const auto& outcome) {
    auto guard = makePostGuard(bat);
    EXPECT_TRUE(outcome.IsSuccess())
      << "createStreamAsync error: "
      << outcome.GetError().GetExceptionName()
      << ": '" << outcome.GetError().GetMessage() << "'";
  });
  ctx->loopOnBaton(bat);
  ctx->waitUntilStreamIsActive(options.streamName);
  UniqueToken token;
  token.mark();
  return StreamGuard(std::move(token), ctx, options);
}

StreamGuard::~StreamGuard() {
  if (token_) {
    token_.clear();
    auto request = DeleteStreamRequest().WithStreamName(
      options_.streamName
    );
    ThreadBaton bat;
    context_->client->deleteStreamAsync(request, [&bat](const auto& outcome) {
      auto guard = makePostGuard(bat);
      EXPECT_TRUE(outcome.IsSuccess());
    });
    context_->loopOnBaton(bat);
    context_->waitUntilStreamDoesNotExist(options_.streamName);
  }
}

} // kclpp_test_support
