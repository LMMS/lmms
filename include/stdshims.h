//! Shims for std:: functions that aren't available in the current C++ versions
//! we target.

#ifndef STDSHIMS_H
#define STDSHIMS_H

#include <memory>
#include <utility>

#if (__cplusplus >= 201402L || _MSC_VER)
#ifndef _MSC_VER
#warning "This file should now be removed! The functions it provides are part of the C++14 standard."
#endif
using std::make_unique;

#else

/// Shim for http://en.cppreference.com/w/cpp/memory/unique_ptr/make_unique
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
	return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
#endif

#endif // include guard

