#include <string>
#include <gtest/gtest.h>
#include <aws/dynamodb/model/AttributeValue.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include "kclpp/leases/ExtendedSequenceNumber.h"

using namespace std;
using kclpp::leases::ExtendedSequenceNumber;
using kclpp::leases::CheckpointID;
using kclpp::leases::SubCheckpointID;

TEST(TestExtendedSequenceNumber, TestEquality) {
  vector<tuple<bool, ExtendedSequenceNumber, ExtendedSequenceNumber>> testCases {
    {
      true,
      ExtendedSequenceNumber { CheckpointID {"001"} },
      ExtendedSequenceNumber { CheckpointID {"001"} }
    },
    {
      false,
      ExtendedSequenceNumber { CheckpointID {"001"} },
      ExtendedSequenceNumber { CheckpointID {"002"} }
    },
    {
      false,
      ExtendedSequenceNumber { CheckpointID {"001"} },
      ExtendedSequenceNumber {
        CheckpointID {"001"},
        SubCheckpointID {"001"}
      }
    },
    {
      true,
      ExtendedSequenceNumber {
        CheckpointID {"001"},
        SubCheckpointID {"010"}
      },
      ExtendedSequenceNumber {
        CheckpointID {"001"},
        SubCheckpointID {"010"}
      }    
    },
    {
      false,
      ExtendedSequenceNumber {
        CheckpointID {"001"},
        SubCheckpointID {"009"}
      },
      ExtendedSequenceNumber {
        CheckpointID {"001"},
        SubCheckpointID {"010"}
      }
    },
    {
      false,
      ExtendedSequenceNumber {
        CheckpointID {"001"},
        SubCheckpointID {"010"}
      },
      ExtendedSequenceNumber {
        CheckpointID {"002"},
        SubCheckpointID {"010"}
      }
    }
  };
  for (const auto& testCase: testCases) {
    auto expected = std::get<0>(testCase);
    const auto& seq1 = std::get<1>(testCase);
    const auto& seq2 = std::get<2>(testCase);
    if (expected) {
      EXPECT_EQ(seq1, seq2);
    } else {
      EXPECT_NE(seq1, seq2);
    }
  }
}
