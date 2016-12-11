#pragma once
#include <aws/dynamodb/DynamoDBClient.h>

namespace kclpp_test_support {

Aws::Client::ClientConfiguration makeClientConfig();

} // kclpp_test_support
