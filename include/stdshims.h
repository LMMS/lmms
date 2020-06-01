//! Shims for std:: functions that aren't available in the current C++ versions
//! we target.

#ifndef STDSHIMS_H
#define STDSHIMS_H

#include <memory>
#include <type_traits>
#include <utility>

#if (__cplusplus >= 201402L || _MSC_VER)
#ifndef _MSC_VER
#warning "This part of this file should now be removed! The functions it provides are part of the C++14 standard."
#endif
using std::make_unique;

#else

/// Shim for http://en.cppreference.com/w/cpp/memory/unique_ptr/make_unique
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
	return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

//! Overload for the case a deleter should be specified
template<typename T, typename Deleter, typename... Args>
std::unique_ptr<T, Deleter> make_unique(Args&&... args)
{
	return std::unique_ptr<T, Deleter>(new T(std::forward<Args>(args)...));
}
#endif

#if (__cplusplus >= 201703L || _MSC_VER >= 1914)
#ifndef _MSC_VER
#warning "This part of this file should now be removed! The functions it provides are part of the C++17 standard."
#endif
using std::as_const;

#else

/// Shim for http://en.cppreference.com/w/cpp/utility/as_const
template <typename T>
constexpr typename std::add_const<T>::type& as_const(T& t) noexcept
{
    return t;
}

template <typename T>
void as_const(const T&&) = delete;
#endif

#endif // include guard

