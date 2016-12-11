#pragma once

namespace kclpp { namespace clientlib { namespace worker {

class WorkerTask {
 public:
  virtual ~WorkerTask() = default;
};

}}} // kclpp::clientlib::worker