#include <glog/logging.h>
#include <gmock/gmock.h>
#include "kclpp/APIGuard.h"

int main(int argc, char** argv) {
  ::testing::InitGoogleMock(&argc, argv);
  google::InstallFailureSignalHandler();
  kclpp::APIGuardFactory factory;
  factory.getOptions().loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Debug;
  auto guard = factory.build();
  return RUN_ALL_TESTS();
}
