/*
 * FileSearch.h
 *
 * Copyright (c) 2025 Sotonye Atemie <sakertooth@gmail.com>
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

#ifndef LMMS_GUI_FILE_SEARCH_H
#define LMMS_GUI_FILE_SEARCH_H

#include <QDir>
#include <QObject>
#include <QString>
#include <future>

namespace lmms::gui {
//! The `FileSearch` class allows for searching for files on the filesystem.
//! Users provide a filter string, which is then parsed and tokenized.
//! Each token is then made into a clause that dictates what it will consider a match.
//! All of the clauses are then combined into a final clause, which is checked against
//! each file as necessary.
//! Searching occurs on a background thread, and results are emitted as a Qt slot back to the user.
class FileSearch : public QObject
{
	Q_OBJECT
public:
	struct Task
	{
		QString filter;
		QStringList paths;
		QStringList extensions;
		QFlags<QDir::Filter> dirFilters;
	};

	FileSearch(QObject* parent = nullptr);
	~FileSearch();

	FileSearch(const FileSearch&) = delete;
	FileSearch(FileSearch&&) = delete;
	FileSearch& operator=(const FileSearch&) = delete;
	FileSearch& operator=(FileSearch&&) = delete;

	//! Search recurisvely within the given @p paths using the specified @p filter.
	//! @p extensions filters out file names that do not have one of the extensions listed.
	//! Any previous searches are overriden as new search requests come in.
	//! Callers can connect to the signals of the search object to respond to the results
	//! as the search is running.
	void search(Task task);

signals:
	//! Emitted when the search object has found a matching path.
	void foundMatch(const QString& path);

	//! Emitted when the search object has finished searching.
	void finished();

private:
	void runSearch(Task task);
	std::future<void> m_task;
	std::atomic_flag m_stop = ATOMIC_FLAG_INIT;
};
} // namespace lmms::gui

#endif // LMMS_GUI_FILE_SEARCH_H