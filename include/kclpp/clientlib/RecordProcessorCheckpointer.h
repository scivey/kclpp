#pragma once
#include "kclpp/clientlib/RecordProcessorCheckpointerIf.h"
#include "kclpp/leases/LeaseCheckpointerIf.h"

namespace kclpp { namespace clientlib {

class RecordProcessorCheckpointer: public RecordProcessorCheckpointerIf {
 protected:
  using lease_checkpointer_ptr_t = std::shared_ptr<LeaseCheckpointerIf>;
  lease_checkpointer_ptr_t leaseCheckpointer_ {nullptr};
 public:
  RecordProcessorCheckpointer(lease_checkpointer_ptr_t leaseCheck)
    : leaseCheckpointer_(leaseCheck) {}

  void checkpoint(const leases::ExtendedSequenceNumber&) override {
    // TODO
    CHECK(false);
  }
};

}} // kclpp::clientlib
