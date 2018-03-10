//! Shims for std:: functions that aren't available in the current C++ versions
//! we target.

#pragma once

/// Shim for http://en.cppreference.com/w/cpp/memory/unique_ptr/make_unique
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
	return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

