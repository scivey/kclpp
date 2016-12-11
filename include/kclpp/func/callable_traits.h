#pragma once
#include <functional>
#include <tuple>

namespace kclpp { namespace func {

namespace detail {

// primary template.
template<class T>
struct callable_traits_base : callable_traits_base<decltype(&T::operator())> {
};

// partial specialization for function type
template<class R, class... Args>
struct callable_traits_base<R(Args...)> {
    using result_type = R;
    using argument_types = std::tuple<Args...>;
};

// partial specialization for function pointer
template<class R, class... Args>
struct callable_traits_base<R (*)(Args...)> {
    using result_type = R;
    using argument_types = std::tuple<Args...>;
};

// partial specialization for std::function
template<class R, class... Args>
struct callable_traits_base<std::function<R(Args...)>> {
    using result_type = R;
    using argument_types = std::tuple<Args...>;
};

// partial specialization for pointer-to-member-function (i.e., operator()'s)
template<class T, class R, class... Args>
struct callable_traits_base<R (T::*)(Args...)> {
    using result_type = R;
    using argument_types = std::tuple<Args...>;
};

template<class T, class R, class... Args>
struct callable_traits_base<R (T::*)(Args...) const> {
    using result_type = R;
    using argument_types = std::tuple<Args...>;
};

// additional cv-qualifier and ref-qualifier combinations omitted
// sprinkle with C-style variadics if desired

} // detail


template<typename ...Types>
struct callable_traits {
  using base_type = detail::callable_traits_base<Types...>;
  using result_type = typename base_type::result_type;
  using argument_types = typename base_type::argument_types;

  template<size_t Idx>
  using nth_arg_type = typename std::tuple_element<Idx, argument_types>::type;

  static const size_t arity = std::tuple_size<argument_types>::value;
};

}} // kclpp::func

