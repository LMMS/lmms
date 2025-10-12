/*
 * SystemSemaphore.cpp
 *
 * Copyright (c) 2024 Dominic Clark
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 */

#include "SystemSemaphore.h"

#include <limits>  // IWYU pragma: keep
#include <system_error>
#include <type_traits>
#include <utility>

#include "lmmsconfig.h"
#include "RaiiHelpers.h"

#ifdef LMMS_HAVE_UNISTD_H
#	include <unistd.h>
#endif

#if (_POSIX_SEMAPHORES > 0 && !defined(__MINGW32__)) || defined(LMMS_BUILD_APPLE)
#	include <fcntl.h>
#	include <semaphore.h>
#elif defined(LMMS_BUILD_WIN32)
#	include <windows.h>
#else
#	error "No system semaphore implementation available"
#endif

namespace lmms {

namespace detail {

#if (_POSIX_SEMAPHORES > 0 && !defined(__MINGW32__)) || defined(LMMS_BUILD_APPLE)

namespace {

[[noreturn]] void throwSystemError(const char* message)
{
	throw std::system_error{errno, std::generic_category(), message};
}

template<typename F>
auto retryWhileInterrupted(F&& function, std::invoke_result_t<F> error = -1)
	noexcept(std::is_nothrow_invocable_v<F>) -> auto
{
	while (true)
	{
		const auto result = function();
		if (result != error || errno != EINTR) { return result; }
	}
}

using UniqueSemaphore = UniqueNullableResource<const char*, nullptr, sem_unlink>;

} // namespace

class SystemSemaphoreImpl
{
public:
	SystemSemaphoreImpl(const std::string& key, unsigned int value) :
		m_key{"/" + key},
		m_sem{retryWhileInterrupted([&]() noexcept {
			return sem_open(m_key.c_str(), O_CREAT | O_EXCL, 0600, value);
		}, SEM_FAILED)}
	{
		if (m_sem == SEM_FAILED) { throwSystemError("SystemSemaphoreImpl: sem_open() failed"); }
		m_ownedSemaphore.reset(m_key.c_str());
	}

	explicit SystemSemaphoreImpl(const std::string& key) :
		m_key{"/" + key},
		m_sem{retryWhileInterrupted([&]() noexcept {
			return sem_open(m_key.c_str(), 0);
		}, SEM_FAILED)}
	{
		if (m_sem == SEM_FAILED) { throwSystemError("SystemSemaphoreImpl: sem_open() failed"); }
	}

	~SystemSemaphoreImpl()
	{
		// We can't use `UniqueNullableResource` for `m_sem`, as the null value
		// (`SEM_FAILED`) is not a constant expression on macOS (it's defined as
		// `(sem_t*) -1`), so can't be used as a template parameter.
		sem_close(m_sem);
	}

	auto acquire() noexcept -> bool
	{
		return retryWhileInterrupted([&]() noexcept {
			return sem_wait(m_sem);
		}) == 0;
	}

	auto release() noexcept -> bool { return sem_post(m_sem) == 0; }

private:
	std::string m_key;
	sem_t* m_sem;
	UniqueSemaphore m_ownedSemaphore;
};

#elif defined(LMMS_BUILD_WIN32)

namespace {

[[noreturn]] void throwSystemError(const char* message)
{
	throw std::system_error{static_cast<int>(GetLastError()), std::system_category(), message};
}

using UniqueHandle = UniqueNullableResource<HANDLE, nullptr, CloseHandle>;

} // namespace

class SystemSemaphoreImpl
{
public:
	SystemSemaphoreImpl(const std::string& key, unsigned int value) :
		m_sem{CreateSemaphoreA(nullptr, value, std::numeric_limits<LONG>::max(), key.c_str())}
	{
		if (!m_sem || GetLastError() == ERROR_ALREADY_EXISTS) {
			throwSystemError("SystemSemaphoreImpl: CreateSemaphoreA failed");
		}
	}

	explicit SystemSemaphoreImpl(const std::string& key) :
		m_sem{OpenSemaphoreA(SEMAPHORE_MODIFY_STATE | SYNCHRONIZE, false, key.c_str())}
	{
		if (!m_sem) { throwSystemError("SystemSemaphoreImpl: OpenSemaphoreA failed"); }
	}

	auto acquire() noexcept -> bool { return WaitForSingleObject(m_sem.get(), INFINITE) == WAIT_OBJECT_0; }
	auto release() noexcept -> bool { return ReleaseSemaphore(m_sem.get(), 1, nullptr); }

private:
	UniqueHandle m_sem;
};

#endif

} // namespace detail

SystemSemaphore::SystemSemaphore() noexcept = default;

SystemSemaphore::SystemSemaphore(std::string key, unsigned int value) :
	m_key{std::move(key)},
	m_impl{std::make_unique<detail::SystemSemaphoreImpl>(m_key, value)}
{}

SystemSemaphore::SystemSemaphore(std::string key) :
	m_key{std::move(key)},
	m_impl{std::make_unique<detail::SystemSemaphoreImpl>(m_key)}
{}

SystemSemaphore::~SystemSemaphore() = default;

SystemSemaphore::SystemSemaphore(SystemSemaphore&& other) noexcept = default;
auto SystemSemaphore::operator=(SystemSemaphore&& other) noexcept -> SystemSemaphore& = default;

auto SystemSemaphore::acquire() noexcept -> bool { return m_impl->acquire(); }
auto SystemSemaphore::release() noexcept -> bool { return m_impl->release(); }

} // namespace lmms
