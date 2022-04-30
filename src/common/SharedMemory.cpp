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
#	include <sys/mman.h>
#	include <sys/stat.h>
#	include <assert.h>
#	include <fcntl.h>
#else
#	include <QtGlobal>
#	include <QSharedMemory>
#endif

namespace detail {

#if _POSIX_SHARED_MEMORY_OBJECTS > 0

namespace {

template<typename F>
int retryWhileInterrupted(F&& function)
{
	int result;
	do
	{
		result = function();
	}
	while (result == -1 && errno == EINTR);
	return result;
}

} // namespace

class SharedMemoryImpl
{
public:
	SharedMemoryImpl() = default;
	SharedMemoryImpl(const SharedMemoryImpl&) = delete;
	SharedMemoryImpl& operator=(const SharedMemoryImpl&) = delete;

	~SharedMemoryImpl()
	{
		if (m_mapping) { munmap(m_mapping, m_size); }
		if (m_isOwner && !m_key.empty()) { shm_unlink(m_key.c_str()); }
	}

	void* attach(const std::string& key, bool readOnly)
	{
		assert(m_mapping == nullptr);
		m_key = "/" + key;
		const auto openFlags = readOnly ? O_RDONLY : O_RDWR;
		const auto fd = retryWhileInterrupted([&] { return shm_open(m_key.c_str(), openFlags, 0); });
		if (fd == -1)
		{
			perror("SharedMemoryImpl::attach: shm_open() failed");
		}
		else
		{
			if (auto stat = (struct stat){}; fstat(fd, &stat) == -1)
			{
				perror("SharedMemoryImpl::attach: fstat() failed");
			}
			else
			{
				m_size = stat.st_size;
				const auto mappingProtection = readOnly ? PROT_READ : PROT_READ | PROT_WRITE;
				const auto mapping = mmap(nullptr, m_size, mappingProtection, MAP_SHARED, fd, 0);
				if (mapping == MAP_FAILED)
				{
					perror("SharedMemoryImpl::attach: mmap() failed");
				}
				else
				{
					m_mapping = mapping;
				}
			}
			retryWhileInterrupted([&] { return close(fd); });
		}
		return m_mapping;
	}

	void* create(const std::string& key, std::size_t size, bool readOnly)
	{
		assert(m_mapping == nullptr);
		m_key = "/" + key;
		m_size = size;
		const auto fd = retryWhileInterrupted([&] { return shm_open(m_key.c_str(), O_RDWR | O_CREAT | O_EXCL, 0600); });
		if (fd == -1)
		{
			perror("SharedMemoryImpl::create: shm_open() failed");
		}
		else
		{
			m_isOwner = true;
			if (retryWhileInterrupted([&] { return ftruncate(fd, m_size); }) == -1)
			{
				perror("SharedMemoryImpl::create: ftruncate() failed");
			}
			else
			{
				const auto mappingProtection = readOnly ? PROT_READ : PROT_READ | PROT_WRITE;
				const auto mapping = mmap(nullptr, m_size, mappingProtection, MAP_SHARED, fd, 0);
				if (mapping == MAP_FAILED)
				{
					perror("SharedMemoryImpl::create: mmap() failed");
				}
				else
				{
					m_mapping = mapping;
				}
			}
			retryWhileInterrupted([&] { return close(fd); });
		}
		return m_mapping;
	}

private:
	std::string m_key;
	void* m_mapping = nullptr;
	std::size_t m_size = 0;
	bool m_isOwner = false;
};

#else

class SharedMemoryImpl
{
public:
	SharedMemoryImpl() = default;
	SharedMemoryImpl(const SharedMemoryImpl&) = delete;
	SharedMemoryImpl& operator=(const SharedMemoryImpl&) = delete;

	void* attach(const std::string& key, bool readOnly)
	{
		const auto mode = readOnly ? QSharedMemory::ReadOnly : QSharedMemory::ReadWrite;
		m_shm.setKey(QString::fromStdString(key));
		const auto success = m_shm.attach(mode);
		return success ? m_shm.data() : nullptr;
	}

	void* create(const std::string& key, std::size_t size, bool readOnly)
	{
		const auto mode = readOnly ? QSharedMemory::ReadOnly : QSharedMemory::ReadWrite;
		m_shm.setKey(QString::fromStdString(key));
		const auto success = m_shm.create(size, mode);
		return success ? m_shm.data() : nullptr;
	}

private:
	QSharedMemory m_shm;
};

#endif

SharedMemoryData::SharedMemoryData()
{ }

SharedMemoryData::SharedMemoryData(std::string&& key, bool readOnly) :
	m_key{std::move(key)},
	m_impl{std::make_unique<SharedMemoryImpl>()},
	m_ptr{m_impl->attach(m_key, readOnly)}
{ }

SharedMemoryData::SharedMemoryData(std::string&& key, std::size_t size, bool readOnly) :
	m_key{std::move(key)},
	m_impl{std::make_unique<SharedMemoryImpl>()},
	m_ptr{m_impl->create(m_key, size, readOnly)}
{ }

SharedMemoryData::~SharedMemoryData() { }

SharedMemoryData::SharedMemoryData(SharedMemoryData&& other) noexcept :
	m_key{std::move(other.m_key)},
	m_impl{std::move(other.m_impl)},
	m_ptr{std::exchange(other.m_ptr, nullptr)}
{ }

} // namespace detail
