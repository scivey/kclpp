#include "kclpp/leases/ExtendedSequenceNumber.h"
#include "kclpp/Optional.h"
#include "kclpp/leases/types.h"

namespace kclpp { namespace leases {

ExtendedSequenceNumber::ExtendedSequenceNumber(){}

ExtendedSequenceNumber::ExtendedSequenceNumber(const CheckpointID& seqNum)
  : sequenceNumber_(seqNum){}

ExtendedSequenceNumber::ExtendedSequenceNumber(const CheckpointID& seqNum, const SubCheckpointID& subSeqNum)
  : sequenceNumber_(seqNum),
    subSequenceNumber_(subSeqNum) {}

const CheckpointID& ExtendedSequenceNumber::sequenceNumber() const {
  return sequenceNumber_;
}

const Optional<SubCheckpointID>& ExtendedSequenceNumber::subSequenceNumber() const {
  return subSequenceNumber_;
}

bool ExtendedSequenceNumber::operator==(const ExtendedSequenceNumber& other) const {
  if (sequenceNumber_ != other.sequenceNumber_) {
    return false;
  }
  if (subSequenceNumber_.hasValue()) {
    if (other.subSequenceNumber_.hasValue()) {
      return subSequenceNumber_.value() == other.subSequenceNumber_.value();
    }
    return false;
  } else if (other.subSequenceNumber_.hasValue()) {
    return false;
  }
  return true;
}

bool ExtendedSequenceNumber::operator!=(const ExtendedSequenceNumber& other) const {
  return ! (*this == other);
}

}} // kclpp::leases
