#include "kclpp/util/util.h"
#include <string>
#include <sstream>
#include <streambuf>
#include <aws/core/utils/DateTime.h>

using namespace std;
using Aws::Utils::DateFormat;
using Aws::Utils::DateTime;

namespace kclpp { namespace util {

string toISO8601(const DateTime& dt) {
  return dt.ToGmtString(DateFormat::ISO_8601);
}

std::vector<std::string> getArgVector(int argc, char** argv) {
  char** current = argv;
  vector<string> result;
  for (int i = 0; i < argc; i++) {
    char *thisOne = *current;
    result.emplace_back(thisOne);
    ++current;
  }
  return result;
}

strtoull_outcome_t safeStrToUll(const std::string& numStr) {
  if (numStr.empty()) {
    return strtoull_outcome_t{DeserializationError{"strtoull on empty string"}};
  }
  size_t result {0};
  int base = 10;
  char *endPtr {nullptr};
  result = std::strtoull(numStr.c_str(), &endPtr, base);
  if (endPtr == numStr.c_str()) {
    return strtoull_outcome_t{DeserializationError{"couldn't read ull from string: " + numStr}};
  }
  if (result == ULLONG_MAX && errno == ERANGE) {
    return strtoull_outcome_t{DeserializationError{"strtoull of out range: " + numStr}};
  }
  return strtoull_outcome_t {result};
}


}} // kclpp::util
