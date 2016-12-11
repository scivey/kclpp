#pragma once

#include <functional>
#include <glog/logging.h>
#include <type_traits>
#include "kclpp/Optional.h"
#include "kclpp/func/callable_traits.h"

namespace kclpp { namespace func {

template<typename TOut, typename...TArgs>
class Function {
 public:
  using func_type = std::function<TOut(TArgs...)>;
  using traits = callable_traits<func_type>;
  using argument_types = typename traits::argument_types;

  template<size_t Idx>
  using nth_arg_type = typename traits::template nth_arg_type<Idx>;

  static const size_t arity = traits::arity;
  using output_type = TOut;
  using func_option_t = kclpp::Optional<func_type>;
 protected:
  func_option_t func_;
 public:
  Function(){}
  bool good() const {
    return !!func_;
  }
  explicit operator bool() const {
    return good();
  }
  Function(const func_type& fn): func_(fn){}
  Function(func_type&& fn): func_(std::move(fn)){}

  Function(const Function& other): func_(other.func_){}
  Function(Function&& other): func_(std::move(other.func_)) {}

  Function& operator=(const Function& other) {
    func_.assign(other.func_);
    return *this;
  }
  Function& operator=(Function&& other) {
    std::swap(func_, other.func_);
    return *this;
  }

  Function& operator=(const func_type& fn) {
    func_.assign(fn);
    return *this;
  }

  Function& operator=(func_type&& fn) {
    func_.assign(std::move(fn));
    return *this;
  }

  template<typename U>
  struct is_func_type {
    static const bool value = std::is_same<
        typename std::remove_reference<U>::type,
        func_type
      >::value || std::is_same<
        typename std::remove_reference<U>::type,
        Function
      >::value;
  };


  template<typename TCallable,
    typename = typename std::enable_if<
      !is_func_type<TCallable>::value,
      TCallable
    >::type
  >
  Function(TCallable&& callable) {
    func_type func {std::forward<TCallable>(callable)};
    func_.assign(std::move(func));
  }

  template<typename TCallable,
    typename = typename std::enable_if<
      !is_func_type<TCallable>::value,
      TCallable
    >::type
  >
  Function& operator=(TCallable&& callable) {
    func_type fn {std::move(callable)};
    func_.assign(std::move(fn));
    return *this;
  }

  template<typename TCallable,
    typename = typename std::enable_if<
      !is_func_type<TCallable>::value,
      TCallable
    >::type
  >
  Function& operator=(const TCallable& callable) {
    func_type fn {callable};
    func_.assign(std::move(fn));
    return *this;
  }

  template<typename ...U>
  output_type call(U&&... args) const {
    DCHECK(good());
    return func_.value()(std::forward<U>(args)...);
  }

  template<typename ...U>
  output_type operator()(U&&... args) const {
    return call(std::forward<U>(args)...);
  }

  using output_option_t = Optional<output_type>;
  template<typename ...U>
  output_option_t callOption(U&&... args) const noexcept {
    output_option_t result;
    if (!good()) {
      return result;
    }
    result.assign(call(std::forward<U>(args)...));
    return result;
  }
};


}} // kclpp::func