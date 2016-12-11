#include <aws/core/utils/Outcome.h>

namespace kclpp {

template<typename R, typename E>
using Outcome = Aws::Utils::Outcome<R, E>;

} // kclpp
