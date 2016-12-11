#pragma once
#include <memory>
#include <atomic>
#include <functional>
#include <vector>

#include "kclpp/dynamo/KCLDynamoClient.h"
#include "kclpp/leases/Lease.h"
#include "kclpp/leases/types.h"
#include "kclpp/clock/Clock.h"
#include "kclpp/leases/LeaseManager.h"
#include "kclpp/locks/Synchronized.h"
#include "kclpp/AsyncGroup.h"

namespace kclpp { namespace async {
class KCLAsyncContext;
}} // kclpp::async

namespace kclpp { namespace leases {


class LeaseRenewerIf {
 public:
  using lease_manager_t = LeaseManager;
  using lease_manager_ptr_t = std::shared_ptr<lease_manager_t>;
  using aws_errors_t = Aws::Client::AWSError<Aws::DynamoDB::DynamoDBErrors>;
  using lease_ptr_t = std::shared_ptr<Lease>;

  // TODO switch the key type here to LeaseKey
  // (need std::hash specialization)
  using lease_map_t = std::map<LeaseKey, lease_ptr_t>;
  using clock_ptr_t = std::shared_ptr<clock::Clock>;
  using NanosecondDuration = clock::NanosecondDuration;
  using NanosecondPoint = clock::NanosecondPoint;

  virtual void addLeasesToRenew(const std::vector<lease_ptr_t> leases) = 0;
  virtual void clearHeldLeases() = 0;
  virtual void dropLease(const lease_ptr_t& lease) = 0;

  using RenewLeaseResult = LeaseManager::RenewLeaseResult;
  using renew_lease_outcome_t = typename LeaseManager::renew_lease_outcome_t;
  using renew_lease_cb_t = std::function<void (const renew_lease_outcome_t&)>;

  struct RenewLeaseOptions {
    bool renewEvenIfExpired {false};
  };
  virtual void renewLease(lease_ptr_t, const RenewLeaseOptions&, renew_lease_cb_t) = 0;

  struct RenewLeasesResult {
    bool succeeded {true};
  };
  using renew_leases_outcome_t = Aws::Utils::Outcome<RenewLeasesResult, aws_errors_t>;
  using renew_leases_cb_t = std::function<void (const renew_leases_outcome_t&)>;
  
  virtual void renewLeases(renew_leases_cb_t callback) = 0;

  virtual lease_map_t copyCurrentlyHeldLeases() = 0;

  virtual Optional<lease_ptr_t> getCurrentlyHeldLease(LeaseKey key) = 0;

  virtual Optional<lease_ptr_t> getCopyOfHeldLease(LeaseKey key, NanosecondPoint nowTime) = 0;

  struct InitializeResult {
    bool success {true};
  };
  using initialize_outcome_t = Aws::Utils::Outcome<InitializeResult, aws_errors_t>;
  using initialize_cb_t = std::function<void(const initialize_outcome_t&)>;
  virtual void initialize(initialize_cb_t callback) = 0;

  using update_lease_outcome_t = LeaseManagerIf::update_lease_outcome_t;
  using update_lease_cb_t = LeaseManagerIf::update_lease_cb_t;
  virtual void updateLease(lease_ptr_t, const ConcurrencyToken&, update_lease_cb_t) = 0;

  virtual ~LeaseRenewerIf() = default;  
};

}} // kclpp::leases