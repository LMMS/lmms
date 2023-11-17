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
	static constexpr int MillisecondsPerMatch = 1;

	class SearchFuture
	{
	public:
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

		auto match() -> QString
		{
			const auto lock = std::lock_guard{m_matchesMutex};
			return m_matches.empty() ? QString{} : m_matches.takeFirst();
		}

		auto state() -> State { return m_state; }
		auto filter() -> const QString& { return m_filter; }
		auto paths() -> const QStringList& { return m_paths; }
		auto extensions() -> const QStringList& { return m_extensions; }

	private:
		auto addMatch(const QString& match) -> void
		{
			const auto lock = std::lock_guard{m_matchesMutex};
			m_matches.append(match);
		}

		QString m_filter;
		QStringList m_paths;
		QStringList m_extensions;

		QStringList m_matches;
		std::mutex m_matchesMutex;

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
	auto process(SearchFuture* searchFuture, const QString& path) -> bool;
	auto pushInBatches(SearchFuture* future, QStringList matches) -> void;

	std::queue<std::shared_ptr<SearchFuture>> m_searchQueue;
	std::atomic<bool> m_cancelRunningSearch = false;

	bool m_workerStopped = false;
	std::mutex m_workerMutex;
	std::condition_variable m_workerCond;
	std::thread m_worker{[this] { run(); }};
};
} // namespace lmms::gui

#endif // LMMS_FILE_BROWSER_SEARCHER_H
