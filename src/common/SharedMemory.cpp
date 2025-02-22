/*
 * SharedMemory.cpp
 *
 * Copyright (c) 2022 Dominic Clark <mrdomclark/at/gmail.com>
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

#include "SharedMemory.h"

#include <random>
#include <system_error>
#include <utility>

#include "lmmsconfig.h"
#include "RaiiHelpers.h"

#ifdef LMMS_HAVE_UNISTD_H
#	include <unistd.h>
#endif

#if _POSIX_SHARED_MEMORY_OBJECTS > 0 || defined(LMMS_BUILD_APPLE)
#	include <sys/mman.h>
#	include <sys/stat.h>
#	include <fcntl.h>
#elif defined(LMMS_BUILD_WIN32)
#	include <windows.h>
#else
#	error "No shared memory implementation available"
#endif

namespace lmms::detail {

#if _POSIX_SHARED_MEMORY_OBJECTS > 0 || defined(LMMS_BUILD_APPLE)

namespace {

[[noreturn]] void throwSystemError(const char* message)
{
	throw std::system_error{errno, std::generic_category(), message};
}

template<typename F>
int retryWhileInterrupted(F&& function) noexcept(std::is_nothrow_invocable_v<F>)
{
	while (true) {
		const auto result = function();
		if (result != -1 || errno != EINTR) { return result; }
	}
}

void deleteFileDescriptor(int fd) noexcept { retryWhileInterrupted([fd]() noexcept { return close(fd); }); }
void deleteShmObject(const char* name) noexcept { shm_unlink(name); }

using FileDescriptor = UniqueNullableResource<int, -1, deleteFileDescriptor>;
using ShmObject = UniqueNullableResource<const char*, nullptr, deleteShmObject>;

} // namespace

class SharedMemoryImpl
{
public:
	SharedMemoryImpl(const std::string& key, bool readOnly) :
		m_key{'/' + key}
	{
		const auto openFlags = readOnly ? O_RDONLY : O_RDWR;
		const auto fd = FileDescriptor{
			retryWhileInterrupted([&]() noexcept { return shm_open(m_key.c_str(), openFlags, 0); })
		};
		if (!fd) { throwSystemError("SharedMemoryImpl: shm_open() failed"); }

		auto stat = (struct stat){};
		if (fstat(fd.get(), &stat) == -1) { throwSystemError("SharedMemoryImpl: fstat() failed"); }
		m_size = stat.st_size;

		const auto mappingProtection = readOnly ? PROT_READ : PROT_READ | PROT_WRITE;
		m_mapping = mmap(nullptr, m_size, mappingProtection, MAP_SHARED, fd.get(), 0);
		if (m_mapping == MAP_FAILED) { throwSystemError("SharedMemoryImpl: mmap() failed"); }
	}

	SharedMemoryImpl(const std::string& key, std::size_t size, bool readOnly) :
		m_key{'/' + key},
		m_size{size}
	{
		const auto fd = FileDescriptor{
			retryWhileInterrupted([&]() noexcept { return shm_open(m_key.c_str(), O_RDWR | O_CREAT | O_EXCL, 0600); })
		};
		if (fd.get() == -1) { throwSystemError("SharedMemoryImpl: shm_open() failed"); }
		m_object.reset(m_key.c_str());

		if (retryWhileInterrupted([&]() noexcept { return ftruncate(fd.get(), m_size); }) == -1) {
			throwSystemError("SharedMemoryImpl: ftruncate() failed");
		}

		const auto mappingProtection = readOnly ? PROT_READ : PROT_READ | PROT_WRITE;
		m_mapping = mmap(nullptr, m_size, mappingProtection, MAP_SHARED, fd.get(), 0);
		if (m_mapping == MAP_FAILED) { throwSystemError("SharedMemoryImpl: mmap() failed"); }
	}

	SharedMemoryImpl(const SharedMemoryImpl&) = delete;
	SharedMemoryImpl& operator=(const SharedMemoryImpl&) = delete;

	~SharedMemoryImpl()
	{
		munmap(m_mapping, m_size);
	}

	auto get() const noexcept -> void* { return m_mapping; }
	auto size_bytes() const noexcept -> std::size_t { return m_size; }

private:
	std::string m_key;
	std::size_t m_size = 0;
	void* m_mapping = nullptr;
	ShmObject m_object;
};

#elif defined(LMMS_BUILD_WIN32)

namespace {

template<typename T>
auto sizeToHighAndLow(T size) -> std::pair<DWORD, DWORD>
{
	if constexpr (sizeof(T) <= sizeof(DWORD)) {
		return {0, size};
	} else {
		return {static_cast<DWORD>(size >> 32), static_cast<DWORD>(size)};
	}
}

[[noreturn]] void throwLastError(const char* message)
{
	throw std::system_error{static_cast<int>(GetLastError()), std::system_category(), message};
}

using UniqueHandle = UniqueNullableResource<HANDLE, nullptr, CloseHandle>;
using FileView = UniqueNullableResource<void*, nullptr, UnmapViewOfFile>;

} // namespace

class SharedMemoryImpl
{
public:
	SharedMemoryImpl(const std::string& key, bool readOnly)
	{
		const auto access = readOnly ? FILE_MAP_READ : FILE_MAP_WRITE;
		m_mapping.reset(OpenFileMappingA(access, false, key.c_str()));
		if (!m_mapping) { throwLastError("SharedMemoryImpl: OpenFileMappingA() failed"); }

		m_view.reset(MapViewOfFile(m_mapping.get(), access, 0, 0, 0));
		if (!m_view) { throwLastError("SharedMemoryImpl: MapViewOfFile() failed"); }

		MEMORY_BASIC_INFORMATION mbi;
		if (VirtualQuery(m_view.get(), &mbi, sizeof(mbi)) == 0)
		{
			throwLastError("SharedMemoryImpl: VirtualQuery() failed");
		}

		m_size = static_cast<std::size_t>(mbi.RegionSize);
	}

	SharedMemoryImpl(const std::string& key, std::size_t size, bool readOnly) :
		m_size{size}
	{
		const auto [high, low] = sizeToHighAndLow(size);
		m_mapping.reset(CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, high, low, key.c_str()));
		// This constructor is supposed to create a new shared memory object,
		// but passing the name of an existing object causes CreateFileMappingA
		// to succeed and return a handle to that object. Thus we have to check
		// GetLastError() too.
		if (!m_mapping || GetLastError() == ERROR_ALREADY_EXISTS) {
			throwLastError("SharedMemoryImpl: CreateFileMappingA() failed");
		}

		const auto access = readOnly ? FILE_MAP_READ : FILE_MAP_WRITE;
		m_view.reset(MapViewOfFile(m_mapping.get(), access, 0, 0, 0));
		if (!m_view) { throwLastError("SharedMemoryImpl: MapViewOfFile() failed"); }
	}

	SharedMemoryImpl(const SharedMemoryImpl&) = delete;
	auto operator=(const SharedMemoryImpl&) -> SharedMemoryImpl& = delete;

	auto get() const noexcept -> void* { return m_view.get(); }
	auto size_bytes() const noexcept -> std::size_t { return m_size; }

private:
	UniqueHandle m_mapping;
	FileView m_view;
	std::size_t m_size = 0;
};

#endif

namespace {

auto createKey() -> std::string
{
	// Max length (minus prepended '/') on macOS (PSHMNAMLEN=31)
	constexpr int length = 30;

	std::string key;
	std::random_device rd;
	auto gen = std::mt19937{rd()}; // mersenne twister, seeded
	auto distrib = std::uniform_int_distribution{0, 15}; // hex range (0-15)

	key.reserve(length + 1);
	for (int i = 0; i < length; ++i)
	{
		key += "0123456789ABCDEF"[distrib(gen)];
	}

	return key;
}

} // namespace

SharedMemoryData::SharedMemoryData() noexcept = default;

SharedMemoryData::SharedMemoryData(std::string&& key, bool readOnly) :
	m_key{std::move(key)},
	m_impl{std::make_unique<SharedMemoryImpl>(m_key, readOnly)},
	m_ptr{m_impl->get()}
{ }

SharedMemoryData::SharedMemoryData(std::string&& key, std::size_t size, bool readOnly) :
	m_key{std::move(key)},
	m_impl{std::make_unique<SharedMemoryImpl>(m_key, std::max(size, std::size_t{1}), readOnly)},
	m_ptr{m_impl->get()}
{ }

SharedMemoryData::SharedMemoryData(std::size_t size, bool readOnly) :
	SharedMemoryData{createKey(), size, readOnly}
{ }

SharedMemoryData::~SharedMemoryData() = default;

SharedMemoryData::SharedMemoryData(SharedMemoryData&& other) noexcept :
	m_key{std::move(other.m_key)},
	m_impl{std::move(other.m_impl)},
	m_ptr{std::exchange(other.m_ptr, nullptr)}
{ }

auto SharedMemoryData::size_bytes() const noexcept -> std::size_t
{
	return m_impl ? m_impl->size_bytes() : 0;
}

} // namespace lmms::detail
