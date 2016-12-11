#pragma once
#include <type_traits>
#include <aws/kinesis/model/Record.h>
#include <string>


namespace kclpp_test_support {

using aws_kinesis_record_data_t = typename std::remove_reference<
  decltype(std::declval<Aws::Kinesis::Model::Record>().GetData())
>::type;

std::string getRecordData(const aws_kinesis_record_data_t& recordData);
std::string getRecordData(const Aws::Kinesis::Model::Record& rec);

} // kclpp_test_support

