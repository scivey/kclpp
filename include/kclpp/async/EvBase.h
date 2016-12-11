#pragma once

#include <memory>
#include <event2/event.h>
#include "kclpp/util/misc.h"
#include "kclpp/TimerSettings.h"

namespace kclpp { namespace async {

class EvBase {
 public:
  using base_t = event_base;
  using base_ptr_t = typename kclpp::util::unique_destructor_ptr<base_t>::type;

 protected:
  base_ptr_t base_ {nullptr};
  void takeOwnershipOfBase(base_t *base);
 public:
  static EvBase createWithOwnership(base_t *base);
  static EvBase create();
  static EvBase* newWithOwnership(base_t *base);
  static EvBase* createNew();

  base_t* getBase();
  void runOnce();
  void runNonBlocking();
  void runForever();
  void runFor(timeval *tv);
  void runFor(const TimerSettings &settings);
};

}} // kclpp::async
