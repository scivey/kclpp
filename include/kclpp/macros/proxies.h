#pragma once

#include <glog/logging.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/core/utils/memory/stl/AWSVector.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include "kclpp/Optional.h"

#define KCLPP_DEFINE_STRING_PROXY_TYPE(class_name) \
  class class_name { \
   protected: \
    Aws::String value_; \
    \
   public: \
    using inner_type = Aws::String; \
    class_name(){} \
    \
    explicit class_name(Aws::String&& value) \
      : value_(std::forward<Aws::String>(value)) {} \
    \
    explicit class_name(const Aws::String& value) \
      : value_(value) {} \
    explicit operator Aws::String() const { \
      return value_; \
    } \
    const Aws::String& value() const { \
      return value_; \
    } \
    bool operator!=(const class_name& other) const { \
      return value() != other.value(); \
    } \
    bool operator==(const class_name& other) const { \
      return value() == other.value(); \
    } \
    bool operator<(const class_name& other) const { \
      return value() < other.value(); \
    } \
  };


#define KCLPP_DEFINE_UNSIGNED_PROXY_TYPE(class_name) \
  class class_name { \
   protected: \
    size_t value_ {0}; \
    \
   public: \
    using inner_type = size_t; \
    class_name(){} \
    \
    explicit class_name(size_t val) \
      : value_(val) {} \
    \
    explicit operator size_t() const { \
      return value_; \
    } \
    size_t value() const { \
      return value_; \
    } \
    bool operator!=(const class_name& other) const { \
      return value() != other.value(); \
    } \
    bool operator==(const class_name& other) const { \
      return value() == other.value(); \
    } \
    bool operator<(const class_name& other) const { \
      return value() < other.value(); \
    } \
  };


#define KCLPP_DEFINE_PROXY_HASH(class_namespace, class_name) \
  template<> \
  struct hash<class_namespace::class_name> { \
    size_t operator()(const class_namespace::class_name& ref) const { \
      return std::hash<typename class_namespace::class_name::inner_type>()(ref.value()); \
    } \
  };
