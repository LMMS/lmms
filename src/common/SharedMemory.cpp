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

#ifndef LMMS_HAVE_SYS_SHM_H
#define USE_QT_SHMEM

#include <QtGlobal>
#include <QSharedMemory>
#endif

namespace detail {

#ifdef USE_QT_SHMEM

class SharedMemoryImpl
{
public:
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

#endif // USE_QT_SHMEM

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
