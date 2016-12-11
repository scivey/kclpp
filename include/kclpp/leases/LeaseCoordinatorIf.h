#pragma once
#include <memory>
#include <atomic>
#include <functional>
#include <vector>

#include "kclpp/leases/Lease.h"
#include "kclpp/leases/types.h"
#include "kclpp/leases/LeaseManager.h"
#include "kclpp/leases/LeaseRenewer.h"
#include "kclpp/leases/LeaseTaker.h"
#include "kclpp/clock/Clock.h"
#include "kclpp/leases/LeaseManager.h"
#include "kclpp/locks/Synchronized.h"
#include "kclpp/AsyncGroup.h"
#include "kclpp/async/KCLAsyncContext.h"
#include "kclpp/util/cas.h"

namespace kclpp { namespace leases {

class LeaseCoordinatorIf {
 public:
  using lease_manager_t = LeaseManager;
  using lease_manager_ptr_t = std::shared_ptr<lease_manager_t>;
  using aws_errors_t = Aws::Client::AWSError<Aws::DynamoDB::DynamoDBErrors>;
  using lease_ptr_t = std::shared_ptr<Lease>;
  using KCLAsyncContext = async::KCLAsyncContext;
  // TODO switch the key type here to LeaseKey
  // (need std::hash specialization)
  using lease_map_t = std::map<std::string, lease_ptr_t>;
  using clock_ptr_t = std::shared_ptr<clock::Clock>;
  using NanosecondDuration = clock::NanosecondDuration;
  using NanosecondPoint = clock::NanosecondPoint;

 public:
  using unit_outcome_t = Outcome<Unit, aws_errors_t>;
  using unit_cb_t = func::Function<void, unit_outcome_t>;
  virtual void start(unit_cb_t callback) = 0;
  virtual LeaseManager* getManager() = 0;
  virtual void stop(unit_cb_t callback) = 0;
  virtual bool isRunning() const = 0;
  using update_lease_cb_t = LeaseRenewerIf::update_lease_cb_t;
  virtual void updateLease(lease_ptr_t, const ConcurrencyToken&, update_lease_cb_t) = 0;
  virtual ~LeaseCoordinatorIf() = default;
  virtual lease_manager_ptr_t getLeaseManager() = 0;
};

}} // kclpp::leases