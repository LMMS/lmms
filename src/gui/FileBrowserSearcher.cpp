/*
 * FileBrowserSearcher.cpp - Batch processor for searching the filesystem
 *
 * Copyright (c) 2023 saker <sakertooth@gmail.com>
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

#include "FileBrowserSearcher.h"

#include <QDir>
#include <stack>

#include "FileBrowser.h"

namespace lmms::gui {

FileBrowserSearcher::~FileBrowserSearcher()
{
	m_cancelRunningSearch = true;

	{
		const auto lock = std::lock_guard{m_workerMutex};
		m_workerStopped = true;
	}

	m_workerCond.notify_one();
	m_worker.join();
}

auto FileBrowserSearcher::search(const QString& filter, const QStringList& paths, const QStringList& extensions)
	-> std::shared_ptr<SearchFuture>
{
	m_cancelRunningSearch = true;
	auto future = std::make_shared<SearchFuture>(filter, paths, extensions);

	{
		const auto lock = std::lock_guard{m_workerMutex};
		m_searchQueue.push(future);
		m_cancelRunningSearch = false;
	}

	m_workerCond.notify_one();
	return future;
}

auto FileBrowserSearcher::run() -> void
{
	while (true)
	{
		auto lock = std::unique_lock{m_workerMutex};
		m_workerCond.wait(lock, [this] { return m_workerStopped || !m_searchQueue.empty(); });

		if (m_workerStopped) { return; }

		const auto future = m_searchQueue.front();
		future->m_state = SearchFuture::State::Running;
		m_searchQueue.pop();

		auto cancelled = false;
		for (const auto& path : future->m_paths)
		{
			if (FileBrowser::directoryBlacklist().contains(path)) { continue; }

			if (!process(future.get(), path))
			{
				future->m_state = SearchFuture::State::Cancelled;
				cancelled = true;
				break;
			}
		}

		if (!cancelled) { future->m_state = SearchFuture::State::Completed; }
	}
}

auto FileBrowserSearcher::process(SearchFuture* searchFuture, const QString& path) -> bool
{
	auto stack = QFileInfoList{};

	auto dir = QDir{path};
	stack.append(dir.entryInfoList(FileBrowser::dirFilters(), FileBrowser::sortFlags()));

	while (!stack.empty())
	{
		if (m_cancelRunningSearch)
		{
			m_cancelRunningSearch = false;
			return false;
		}

		const auto info = stack.takeFirst();
		const auto path = info.absoluteFilePath();
		if (FileBrowser::directoryBlacklist().contains(path)) { continue; }

		const auto name = info.fileName();
		const auto validFile = info.isFile() && searchFuture->m_extensions.contains(info.suffix(), Qt::CaseInsensitive);
		const auto passesFilter = name.contains(searchFuture->m_filter, Qt::CaseInsensitive);

		// Only when a directory doesn't pass the filter should we search further
		if (info.isDir() && !passesFilter)
		{
			dir.setPath(path);
			auto entries = dir.entryInfoList(FileBrowser::dirFilters(), FileBrowser::sortFlags());

			// Reverse to maintain the sorting within this directory when popped
			std::reverse(entries.begin(), entries.end());

			for (const auto& entry : entries)
			{
				stack.push_front(entry);
			}
		}
		else if ((validFile || info.isDir()) && passesFilter) { searchFuture->addMatch(path); }
	}
	return true;
}

} // namespace lmms::gui
