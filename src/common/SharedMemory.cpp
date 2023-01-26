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

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_UNISTD_H
#	include <unistd.h>
#endif

#if _POSIX_SHARED_MEMORY_OBJECTS > 0
#	include <system_error>
#
#	include <sys/mman.h>
#	include <sys/stat.h>
#	include <fcntl.h>
#
#	include "RaiiHelpers.h"
#else
#	include <stdexcept>
#
#	include <QtGlobal>
#	include <QSharedMemory>
#endif


namespace lmms::detail {

#if _POSIX_SHARED_MEMORY_OBJECTS > 0


namespace {

template<typename F>
int retryWhileInterrupted(F&& function) noexcept(std::is_nothrow_invocable_v<F>)
{
	int result;
	do
	{
		result = function();
	}
	while (result == -1 && errno == EINTR);
	return result;
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
		m_key{"/" + key}
	{
		const auto openFlags = readOnly ? O_RDONLY : O_RDWR;
		const auto fd = FileDescriptor{
			retryWhileInterrupted([&]() noexcept { return shm_open(m_key.c_str(), openFlags, 0); })
		};
		if (!fd)
		{
			throw std::system_error{errno, std::generic_category(), "SharedMemoryImpl: shm_open() failed"};
		}
		auto stat = (struct stat){};
		if (fstat(fd.get(), &stat) == -1)
		{
			throw std::system_error{errno, std::generic_category(), "SharedMemoryImpl: fstat() failed"};
		}
		m_size = stat.st_size;
		const auto mappingProtection = readOnly ? PROT_READ : PROT_READ | PROT_WRITE;
		m_mapping = mmap(nullptr, m_size, mappingProtection, MAP_SHARED, fd.get(), 0);
		if (m_mapping == MAP_FAILED)
		{
			throw std::system_error{errno, std::generic_category(), "SharedMemoryImpl: mmap() failed"};
		}
	}

	SharedMemoryImpl(const std::string& key, std::size_t size, bool readOnly) :
		m_key{"/" + key},
		m_size{size}
	{
		const auto fd = FileDescriptor{
			retryWhileInterrupted([&]() noexcept { return shm_open(m_key.c_str(), O_RDWR | O_CREAT | O_EXCL, 0600); })
		};
		if (fd.get() == -1)
		{
			throw std::system_error{errno, std::generic_category(), "SharedMemoryImpl: shm_open() failed"};
		}
		m_object.reset(m_key.c_str());
		if (retryWhileInterrupted([&]() noexcept { return ftruncate(fd.get(), m_size); }) == -1)
		{
			throw std::system_error{errno, std::generic_category(), "SharedMemoryImpl: ftruncate() failed"};
		}
		const auto mappingProtection = readOnly ? PROT_READ : PROT_READ | PROT_WRITE;
		m_mapping = mmap(nullptr, m_size, mappingProtection, MAP_SHARED, fd.get(), 0);
		if (m_mapping == MAP_FAILED)
		{
			throw std::system_error{errno, std::generic_category(), "SharedMemoryImpl: mmap() failed"};
		}
	}

	SharedMemoryImpl(const SharedMemoryImpl&) = delete;
	SharedMemoryImpl& operator=(const SharedMemoryImpl&) = delete;

	~SharedMemoryImpl()
	{
		munmap(m_mapping, m_size);
	}

	void* get() { return m_mapping; }

private:
	std::string m_key;
	std::size_t m_size;
	void* m_mapping;
	ShmObject m_object;
};

#else

class SharedMemoryImpl
{
public:
	SharedMemoryImpl(const std::string& key, bool readOnly) :
		m_shm{QString::fromStdString(key)}
	{
		const auto mode = readOnly ? QSharedMemory::ReadOnly : QSharedMemory::ReadWrite;
		if (!m_shm.attach(mode))
		{
			throw std::runtime_error{"SharedMemoryImpl: QSharedMemory::attach() failed"};
		}
	}

	SharedMemoryImpl(const std::string& key, std::size_t size, bool readOnly) :
		m_shm{QString::fromStdString(key)}
	{
		const auto mode = readOnly ? QSharedMemory::ReadOnly : QSharedMemory::ReadWrite;
		if (!m_shm.create(size, mode))
		{
			throw std::runtime_error{"SharedMemoryImpl: QSharedMemory::create() failed"};
		}
	}

	SharedMemoryImpl(const SharedMemoryImpl&) = delete;
	SharedMemoryImpl& operator=(const SharedMemoryImpl&) = delete;

	void* get() { return m_shm.data(); }

private:
	QSharedMemory m_shm;
};

#endif

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

SharedMemoryData::~SharedMemoryData() = default;

SharedMemoryData::SharedMemoryData(SharedMemoryData&& other) noexcept :
	m_key{std::move(other.m_key)},
	m_impl{std::move(other.m_impl)},
	m_ptr{other.m_ptr}
{ }

} // namespace lmms::detail
