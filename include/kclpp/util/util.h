#pragma once
#include <string>

namespace Aws { namespace Utils {

class DateTime;

}} // Aws::Utils


namespace awscratch { namespace util {

std::string toISO8601(const Aws::Utils::DateTime&);

}} // awscratch::util

