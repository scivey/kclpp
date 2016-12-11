#pragma once
#include <string>
#include <vector>
#include <set>
#include <map>
#include <random>
#include <algorithm>
#include <cstdint>
#include <climits>

#include <errno.h>

#include <aws/core/utils/Outcome.h>
#include "kclpp/KCLPPError.h"
#include "kclpp/Optional.h"

namespace Aws { namespace Utils {

class DateTime;

}} // Aws::Utils


namespace kclpp { namespace util {

std::string toISO8601(const Aws::Utils::DateTime&);

std::vector<std::string> getArgVector(int argc, char** argv);

using strtoull_outcome_t = Aws::Utils::Outcome<size_t, DeserializationError>;
strtoull_outcome_t safeStrToUll(const std::string& numStr);

template<typename TMap,
  typename = typename TMap::key_type,
  typename = typename TMap::mapped_type>
std::set<typename TMap::key_type> keySet(const TMap& someMap) {
  using key_type = typename TMap::key_type;
  std::set<key_type> result;
  for (const auto& keyVal: someMap) {
    result.insert(keyVal.first);
  }
  return result;
}

template<typename TMap,
  typename = typename TMap::key_type,
  typename = typename TMap::mapped_type>
Optional<typename TMap::mapped_type> findOption(const TMap& someMap,
    const typename TMap::key_type& key) {
  auto found = someMap.find(key);
  Optional<typename TMap::mapped_type> result;
  if (found != someMap.end()) {
    result.assign(found->second);
  }
  return result;
}

template<typename TMap,
  typename = typename TMap::key_type,
  typename = typename TMap::mapped_type>
void insertOrUpdate(TMap& someMap, const typename TMap::key_type& key,
    typename TMap::mapped_type value) {
  using val_type = typename TMap::mapped_type;
  auto found = someMap.find(key);
  if (found == someMap.end()) {
    someMap.insert(std::make_pair(key, std::forward<val_type>(value)));
  } else {
    found->second = std::forward<val_type>(value);
  }
}

template<typename TMap,
  typename = typename TMap::key_type,
  typename = typename TMap::mapped_type>
void insertOrIncrement(TMap& someMap, const typename TMap::key_type& key,
      typename TMap::mapped_type value) {
  using val_type = typename TMap::mapped_type;
  {
    auto found = someMap.find(key);
    if (found == someMap.end()) {
      someMap.insert(std::make_pair(key, val_type{0}));
    }
  }
  auto found = someMap.find(key);
  found->second += value;
}

template<typename TAssocContainer,
  typename = typename TAssocContainer::key_type>
bool removeIfExists(TAssocContainer& container,
    const typename TAssocContainer::key_type& key) {
  if (container.count(key) == 0) {
    return false;
  }
  container.erase(key);
  return true;
}

template<typename TContainer>
void randomShuffle(TContainer& container) {
  std::random_device rd;
  std::mt19937 engine {rd()};
  std::shuffle(container.begin(), container.end(), engine);
}


}} // kclpp::util

