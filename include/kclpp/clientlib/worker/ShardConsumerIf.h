#pragma once

namespace kclpp { namespace clientlib {

class ShutdownNotification;

namespace record_processor {
class RecordProcessor;
}

class RecordProcessorCheckpointer;
class Record;

}} // kclpp::clientlib

namespace kclpp { namespace clientlib { namespace worker {

class ShardConsumerIf {
 public:
  // virtual std::shared_ptr<RecordProcessorCheckpointer> getRecordProcessorCheckpointer() = 0;
  // virtual std::shared_ptr<RecordProcessor> getRecordProcessor() = 0;
  // virtual std::shared_ptr<StreamConfig> getStreamConfig() = 0;
  // virtual std::shared_ptr<ConsumerSates> getStreamConfig() = 0;
  // virtual std::chrono::milliseconds getTaskBackoffTime() = 0;
  // virtual std::chrono::milliseconds getParentShardPollInterval() = 0;
  // virtual ShutdownNotification getShutdownNotification() = 0;
  // virtual bool isCleanupLeasesOfCompletedShards() = 0;
  virtual ~ShardConsumerIf() = default;
};

}}} // kclpp::clientlib::worker