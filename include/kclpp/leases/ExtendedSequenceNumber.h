#pragma once

#include "kclpp/Optional.h"
#include "kclpp/leases/types.h"

namespace kclpp { namespace leases {

class ExtendedSequenceNumber {
 protected:
  CheckpointID sequenceNumber_;
  Optional<SubCheckpointID> subSequenceNumber_;
 public:
  ExtendedSequenceNumber(const CheckpointID&);
  ExtendedSequenceNumber(const CheckpointID& seqNum, const SubCheckpointID& subSeqNum);
  ExtendedSequenceNumber();
  const CheckpointID& sequenceNumber() const;
  const Optional<SubCheckpointID>& subSequenceNumber() const;
  bool operator==(const ExtendedSequenceNumber& other) const;
  bool operator!=(const ExtendedSequenceNumber& other) const;
};

}} // kclpp::leases
