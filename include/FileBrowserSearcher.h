/*
 * FileBrowserSearcher.h - Batch processor for searching the filesystem
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

#ifndef LMMS_FILE_BROWSER_SEARCHER_H
#define LMMS_FILE_BROWSER_SEARCHER_H

#include <QHash>
#include <QString>
#include <QStringList>
#include <QUuid>
#include <optional>
#include <queue>

#ifdef __MINGW32__
#include <mingw.condition_variable.h>
#include <mingw.mutex.h>
#include <mingw.thread.h>
#else
#include <condition_variable>
#include <mutex>
#include <thread>
#endif

namespace lmms::gui {
class FileBrowserSearcher
{
public:
	static constexpr int s_batchSize = 256;
	static constexpr int s_millisecondsPerBatch = 250;

	//! The future that will be returned to the requester, as well as read and modified by both the requester and
	//! worker thread. Acessing the filter, paths to search, and valid file extensions can be done concurrently since
	//! they do not change after construction. Access to the batch queue is protected under a mutex, while the state
	//! variable is atomic.
	class SearchFuture
	{
	public:
		//! The state for this future. The state of this object can be read/written atomically. In a cancelled
		//! state, the requester doesn't need to continue using this future and can abort any operations made using it.
		//! Once in a completed or cancelled state, the worker thread no longer has shared ownership of this future
		//! (i.e., all ownership will go to the requester).
		enum class State
		{
			Idle,
			Running,
			Cancelled,
			Completed
		};

		SearchFuture(const QString& filter, const QStringList& paths, const QStringList& extensions)
			: m_filter(filter)
			, m_paths(paths)
			, m_extensions(extensions)
		{
		}

		//! Read the current state of the future object. This function can run concurrently with the worker thread.
		auto state() -> State { return m_state; }

		//! Read a batch of search results. This function can run concurrently with the worker thread. Returns a empty
		//! list if no batches are available.
		auto batch() -> QStringList;

		auto filter() -> const QString& { return m_filter; }
		auto paths() -> const QStringList& { return m_paths; }
		auto extensions() -> const QStringList& { return m_extensions; }

	private:
		auto addBatch(QStringList& matches) -> void;

		QString m_filter;
		QStringList m_paths;
		QStringList m_extensions;

		std::list<QStringList> m_batchQueue;
		std::mutex m_batchQueueMutex;

		std::atomic<State> m_state = State::Idle;

		friend FileBrowserSearcher;
	};

	~FileBrowserSearcher();

	//! Enqueues a search to be ran by the worker thread.
	//! Returns a future that the caller can use to track state and results of the operation.
	auto search(const QString& filter, const QStringList& paths, const QStringList& extensions)
		-> std::shared_ptr<SearchFuture>;

	//! Sends a signal to cancel a running search.
	auto cancel() -> void { m_cancelRunningSearch = true; }

	//! Returns the global instance of the searcher object.
	static auto instance() -> FileBrowserSearcher*
	{
		static auto s_instance = FileBrowserSearcher{};
		return &s_instance;
	}

private:
	auto run() -> void;
	auto process(std::shared_ptr<SearchFuture> searchFuture) -> void;

	std::queue<std::shared_ptr<SearchFuture>> m_searchQueue;
	std::atomic<bool> m_cancelRunningSearch = false;

	bool m_workerStopped = false;
	std::mutex m_workerMutex;
	std::condition_variable m_workerCond;
	std::thread m_worker{[this] { run(); }};
};
} // namespace lmms::gui

#endif // LMMS_FILE_BROWSER_SEARCHER_H
