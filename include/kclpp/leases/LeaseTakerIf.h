#pragma once
#include <memory>
#include <atomic>
#include <functional>

#include "kclpp/dynamo/KCLDynamoClient.h"
#include "kclpp/leases/Lease.h"
#include "kclpp/leases/types.h"
#include "kclpp/clock/Clock.h"
#include "kclpp/leases/LeaseManager.h"
#include "kclpp/locks/Synchronized.h"
#include "kclpp/Unit.h"

namespace kclpp { namespace async {
class KCLAsyncContext;
}} // kclpp::async


namespace kclpp { namespace leases {

class LeaseTakerIf {
 public:
  using lease_manager_t = LeaseManager;
  using lease_manager_ptr_t = std::shared_ptr<lease_manager_t>;
  using aws_errors_t = Aws::Client::AWSError<Aws::DynamoDB::DynamoDBErrors>;
  using lease_ptr_t = std::shared_ptr<Lease>;
  using lease_map_t = std::map<LeaseKey, lease_ptr_t>;
  using lease_count_map_t = std::map<LeaseOwner, size_t>;
  using clock_ptr_t = std::shared_ptr<clock::Clock>;
  using NanosecondDuration = clock::NanosecondDuration;
  using NanosecondPoint = clock::NanosecondPoint;

  struct TakeLeasesResult {
    std::vector<lease_ptr_t> leases;
  };
  using take_leases_outcome_t = Aws::Utils::Outcome<TakeLeasesResult, aws_errors_t>;
  using take_leases_cb_t = std::function<void (const take_leases_outcome_t&)>;
  virtual void takeLeases(take_leases_cb_t) = 0;

  using update_all_leases_outcome_t = Aws::Utils::Outcome<Unit, aws_errors_t>;
  using update_all_leases_cb_t = std::function<void (const update_all_leases_outcome_t&)>;
  virtual void updateAllLeases(update_all_leases_cb_t) = 0;

  virtual std::vector<lease_ptr_t> getExpiredLeases() = 0;

  virtual lease_count_map_t computeLeaseCounts(const std::set<LeaseKey>& expiredLeaseKeys) = 0;

  struct ChooseLeasesToStealOptions {
    NumberOfLeases neededNum {0};
    NumberOfLeases targetNum {0};
  };
  virtual std::vector<lease_ptr_t> chooseLeasesToSteal(
    const lease_count_map_t&,
    const ChooseLeasesToStealOptions&
  ) = 0;
  
  virtual std::vector<lease_ptr_t> computeLeasesToTake(const std::vector<lease_ptr_t>& expiredLeases) = 0;

  virtual ~LeaseTakerIf() = default;
};

}} // kclpp::leases