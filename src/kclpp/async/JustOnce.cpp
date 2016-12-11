#include "kclpp/async/JustOnce.h"

namespace kclpp { namespace async {

JustOnce::JustOnce(cb_t&& func): func_(std::forward<cb_t>(func)) {}

bool JustOnce::run() {
  if (!ran_) {
    ran_ = true;
    func_();
    return true;
  }
  return false;
}

void JustOnce::operator()() {
  run();
}

}} // kclpp::async

