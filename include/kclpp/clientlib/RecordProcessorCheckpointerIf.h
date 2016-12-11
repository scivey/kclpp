#pragma once
#include <aws/kinesis/model/Record.h>
#include "kclpp/leases/types.h"
#include "kclpp/leases/ExtendedSequenceNumber.h"

namespace kclpp { namespace clientlib {

class RecordProcessorCheckpointerIf {
 public:
  using Record = Aws::Kinesis::Model::Record;
  virtual void checkpoint(const Record& record) {
    checkpoint(leases::CheckpointID{record.GetSequenceNumber()});
  }
  virtual void checkpoint(const leases::CheckpointID& seqId) {
    checkpoint(leases::ExtendedSequenceNumber { seqId });
  }
  virtual void checkpoint(const leases::CheckpointID& seqId,
      const leases::SubCheckpointID& subId) {
    checkpoint(leases::ExtendedSequenceNumber{
      seqId, subId
    });
  }
  virtual void checkpoint(const leases::ExtendedSequenceNumber&) = 0;
  virtual ~RecordProcessorCheckpointerIf() = default;
};

}} // kclpp::clientlib
