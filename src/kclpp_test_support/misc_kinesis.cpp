#include "kclpp_test_support/misc_kinesis.h"
#include <type_traits>
#include <aws/kinesis/model/Record.h>
#include <string>

using namespace std;

namespace kclpp_test_support {

std::string getRecordData(const aws_kinesis_record_data_t& recordData) {
  return string {
    (const char*) recordData.GetUnderlyingData(),
    recordData.GetLength()
  };  
}

std::string getRecordData(const Aws::Kinesis::Model::Record& rec) {
  const auto& recordData = rec.GetData();
  return getRecordData(recordData);
}

} // kclpp_test_support

