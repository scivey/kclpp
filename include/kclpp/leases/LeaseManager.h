#pragma once

#include "kclpp/dynamo/KCLDynamoClient.h"
#include "kclpp/leases/Lease.h"
#include "kclpp/async/KCLAsyncContext.h"
#include "kclpp/leases/LeaseManagerIf.h"
#include "kclpp/dynamo/types.h"
#include "kclpp/leases/dynamo_helpers.h"

namespace kclpp { namespace leases {

class LeaseManager: public LeaseManagerIf {
 public:
  struct LeaseManagerParams {
    dynamo_client_ptr_t dynamoClient {nullptr};
    dynamo::TableName tableName;
    bool consistentReads {false};
    std::shared_ptr<async::KCLAsyncContext> asyncContext {nullptr};
  };

  struct LeaseManagerState {
    LeaseManagerParams params;
  };

  using state_ptr_t = std::unique_ptr<LeaseManagerState>;

 protected:
  state_ptr_t state_;
  LeaseManager(state_ptr_t&& state);

 public:
  using create_shared_outcome_t = Aws::Utils::Outcome<
    std::shared_ptr<LeaseManager>, kclpp::InvalidState
  >;
  static create_shared_outcome_t createShared(LeaseManagerParams&& params);

  void createTableIfNotExists(const CreateTableParams&, create_table_cb_t) override;
  void listLeases(list_leases_cb_t callback) override;

 protected:
  void handleDeleteResponse(const DeleteItemOutcome&, delete_lease_cb_t callback);

 public:
  void deleteLeaseCAS(lease_ptr_t lease, delete_lease_cb_t callback) override;
  void deleteLeaseUnconditionally(lease_ptr_t lease, delete_lease_cb_t callback) override;

 protected:
  using internal_update_outcome_t = typename KCLDynamoClient::update_item_outcome_t;
  using internal_update_cb_t = std::function<void(
      const internal_update_outcome_t&, const UpdateItemRequest&
  )>;

  struct InternalUpdateLeaseParams {
    lease_ptr_t lease {nullptr};
    dynamo_helpers::UpdateLeaseParams updateParams;
  };

  // common error handling in initial callback layer
  void internalUpdateLease(const InternalUpdateLeaseParams&, internal_update_cb_t);

 public:
  void updateLease(lease_ptr_t lease, update_lease_cb_t callback) override;
  void isLeaseTableEmpty(is_table_empty_cb_t callback) override;
  void createLeaseIfNotExists(lease_ptr_t lease, create_lease_cb_t callback) override;
  void getLease(LeaseKey key, get_lease_cb_t callback) override;
  void renewLease(lease_ptr_t lease, renew_lease_cb_t callback) override;
  void evictLease(lease_ptr_t lease, evict_lease_cb_t callback) override;
  void takeLease(lease_ptr_t lease, LeaseOwner newOwner, take_lease_cb_t callback) override;
  void waitUntilLeaseTableExists(const WaitUntilTableExistsParams&, wait_until_exists_cb_t) override;
};

}} // kclpp::leases