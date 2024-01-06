/*
 * FileSystemEvictor.h
 *
 * Copyright (c) 2024 saker <sakertooth@gmail.com>
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
 *
 */

#ifndef LMMS_FILE_SYSTEM_EVICTOR_H
#define LMMS_FILE_SYSTEM_EVICTOR_H

#include <QFileSystemWatcher>

#include "SampleCache.h"

namespace lmms {
class FileSystemEvictor : public SampleCache::Evictor
{
public:
	virtual ~FileSystemEvictor() = default;

private:
    //! Setup signal to evict entries on changes within the file system.
    virtual void setup(SampleCache& cache) override;

	//! Treats `key` as a path to a sample on the filesystem and evicts the added entry if the file changes.
	virtual void notifyAdd(const std::string& key) override;

private:
	QFileSystemWatcher m_watcher;
};
} // namespace lmms

#endif // LMMS_FILE_SYSTEM_EVICTOR_H
