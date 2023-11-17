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
		process(std::move(future));
	}
}

auto FileBrowserSearcher::process(std::shared_ptr<SearchFuture> searchFuture) -> void
{
	auto matches = QStringList{};
	auto queue = std::queue<QString>{};

	for (const auto& path : searchFuture->paths())
	{
		queue.push(path);
	}

	auto dir = QDir{};
	while (!queue.empty())
	{
		const auto path = queue.front();
		queue.pop();

		dir.setPath(path);
		for (const auto& entry : dir.entryInfoList(FileBrowser::dirFilters(), FileBrowser::sortFlags()))
		{
			const auto name = entry.fileName();
			const auto path = entry.absoluteFilePath();
			const auto validFile
				= entry.isFile() && searchFuture->extensions().contains(entry.suffix(), Qt::CaseInsensitive);
			const auto passesFilter = name.contains(searchFuture->filter(), Qt::CaseInsensitive);

			if ((validFile || entry.isDir()) && passesFilter) { matches.push_back(path); }
			if (entry.isDir() && !passesFilter) { queue.push(path); }

			if (m_cancelRunningSearch)
			{
				m_cancelRunningSearch = false;
				searchFuture->m_state = SearchFuture::State::Cancelled;
				return;
			}
		}

		pushInBatches(searchFuture.get(), std::move(matches));
	}

	searchFuture->m_state = SearchFuture::State::Completed;
}

auto FileBrowserSearcher::pushInBatches(SearchFuture* future, QStringList matches) -> void
{
	const auto batchLock = std::lock_guard{future->m_batchQueueMutex};
	while (!matches.empty())
	{
		const auto batchSize = std::min(FileBrowserSearcher::BatchSize, matches.size());
		const auto batch = QStringList{matches.begin(), matches.begin() + batchSize};
		future->m_batchQueue.push_front(matches);
		matches = QStringList{matches.begin() + batchSize, matches.end()};
	}
}

auto FileBrowserSearcher::SearchFuture::batch() -> QStringList
{
	const auto batchLock = std::lock_guard{m_batchQueueMutex};
	if (m_batchQueue.empty()) { return QStringList{}; }

	const auto batch = m_batchQueue.front();
	m_batchQueue.pop_front();
	return batch;
}

} // namespace lmms::gui
