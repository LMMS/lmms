/*
 * FileSearch.cpp - File system search task
 *
 * Copyright (c) 2024 saker
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

#include "FileSearch.h"

#include <atomic>
#include <chrono>
#include <lmmsconfig.h>
#include <ratio>
#include <thread>

namespace lmms {
FileSearch::FileSearch(const QString& filter, const QStringList& paths, const QStringList& extensions,
	const QStringList& blacklist, QDir::Filters dirFilters, QDir::SortFlags sortFlags)
	: m_filter(filter)
	, m_paths(paths)
	, m_extensions(extensions)
	, m_blacklist(blacklist)
	, m_dirFilters(dirFilters)
	, m_sortFlags(sortFlags)
{
}

void FileSearch::operator()()
{
	auto stack = QFileInfoList{};
	for (const auto& path : m_paths)
	{
		if (m_blacklist.contains(path)) { continue; }

		auto dir = QDir{path};
		stack.append(dir.entryInfoList(m_dirFilters, m_sortFlags));

		while (!stack.empty())
		{
			if (m_cancel.load(std::memory_order_relaxed)) { return; }

			const auto info = stack.takeFirst();
			const auto entryPath = info.absoluteFilePath();
			const auto name = info.fileName();
			const auto validFile = info.isFile() && m_extensions.contains(info.suffix(), Qt::CaseInsensitive);
			const auto passesFilter = name.contains(m_filter, Qt::CaseInsensitive);

			// Only when a directory doesn't pass the filter should we search further
			if (info.isDir() && !passesFilter)
			{
				dir.setPath(entryPath);
				auto entries = dir.entryInfoList(m_dirFilters, m_sortFlags);

				// Reverse to maintain the sorting within this directory when popped
				std::reverse(entries.begin(), entries.end());

				for (const auto& entry : entries)
				{
					stack.push_front(entry);
				}
			}
			else if ((validFile || info.isDir()) && passesFilter)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds{MillisecondsBetweenResults});
				emit foundMatch(this, entryPath);
			}
		}
	}

	emit searchCompleted(this);
}

void FileSearch::cancel()
{
	m_cancel.store(true, std::memory_order_relaxed);
}

} // namespace lmms