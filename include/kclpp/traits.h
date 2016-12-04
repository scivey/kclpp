#pragma once
#include <type_traits>

namespace kclpp {

template<typename T>
struct value_type_of {
  using real_type = typename std::remove_reference<T>::type;
  using type = typename real_type::value_type;
};

// primary template.
template<class T>
struct function_traits : function_traits<decltype(&T::operator())> {
};

// partial specialization for function type
template<class R, class... Args>
struct function_traits<R(Args...)> {
    using result_type = R;
    using argument_types = std::tuple<Args...>;
};

// partial specialization for function pointer
template<class R, class... Args>
struct function_traits<R (*)(Args...)> {
    using result_type = R;
    using argument_types = std::tuple<Args...>;
};

// partial specialization for std::function
template<class R, class... Args>
struct function_traits<std::function<R(Args...)>> {
    using result_type = R;
    using argument_types = std::tuple<Args...>;
};

// partial specialization for pointer-to-member-function (i.e., operator()'s)
template<class T, class R, class... Args>
struct function_traits<R (T::*)(Args...)> {
    using result_type = R;
    using argument_types = std::tuple<Args...>;
};

template<class T, class R, class... Args>
struct function_traits<R (T::*)(Args...) const> {
    using result_type = R;
    using argument_types = std::tuple<Args...>;
};

template<class T>
using first_argument_type = typename std::tuple_element<0, typename function_traits<T>::argument_types>::type;

} // kclpp


#define KCLPP_RESULT_TYPE(expr) \
  std::remove_reference<decltype(expr)>::type

#define KCLPP_ELEMENT_TYPE(expr) \
  value_type_of<decltype(expr)>::type

