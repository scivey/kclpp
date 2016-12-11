#pragma once
#include <functional>
#include <memory>

namespace kclpp { namespace util {

template<typename T>
struct destructor_type {
  using type = std::function<void(T*)>;
};

template<typename T>
struct unique_destructor_ptr {
  using type = std::unique_ptr<T, typename destructor_type<T>::type>;
};

template<typename T, typename TCallable>
auto asDestructorPtr(T *ptr, TCallable &&callable) -> typename unique_destructor_ptr<T>::type {
  using uniq_t = typename unique_destructor_ptr<T>::type;
  return uniq_t {ptr, std::forward<TCallable>(callable)};
}

template<typename T, typename TCallable>
auto asDestructorPtr(T *ptr, const TCallable &callable) -> typename unique_destructor_ptr<T>::type {
  using uniq_t = typename unique_destructor_ptr<T>::type;
  return uniq_t {ptr, callable};
}

template<typename T, typename... Args>
std::unique_ptr<T> makeUnique(Args&&... args) {
  return std::unique_ptr<T> {
    new T {std::forward<Args>(args)...}
  };
}

template<typename T, template<class...> class TSmartPtr, typename ...Types>
TSmartPtr<T> createSmart(Types&&... args) {
  return TSmartPtr<T> { T::createNew(std::forward<Types>(args)...) };
}

template<typename T, typename ...Types>
std::unique_ptr<T> createUnique(Types&&... args) {
  return createSmart<T, std::unique_ptr, Types...>(
    std::forward<Types>(args)...
  );
}

template<typename T, typename ...Types>
std::shared_ptr<T> createShared(Types&&... args) {
  return createSmart<T, std::shared_ptr, Types...>(
    std::forward<Types>(args)...
  );
}

}} // kclpp::util