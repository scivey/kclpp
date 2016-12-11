#pragma once
#include "kclpp/kinesis/types.h"
#include "kclpp/clientlib/worker/InitialPositionInStream.h"

namespace kclpp { namespace clientlib { namespace worker {


class StreamConfig {
 public:
  struct StreamConfigState {
    kinesis::StreamName streamName;
    kinesis::NumberOfRecords maxRecordsPerCall {500};
    std::chrono::milliseconds idleTime {10};
    bool callProcessRecordsIfEmpty {false};
    bool validateSequenceNumberBeforeCheckpointing {false};
    InitialPositionInStream initialPosition {InitialPositionInStream::TRIM_HORIZON};
  };
 protected:
  StreamConfigState state_;
 public:
  StreamConfig(){}
  StreamConfig(StreamConfigState&& initialState)
    : state_(std::forward<StreamConfigState>(initialState)) {}
  const StreamConfigState& getState() const {
    return state_;
  }
};

}}} // kclpp::clientlib::worker