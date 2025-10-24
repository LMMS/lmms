/*
 * FileSearch.h
 *
 * Copyright (c) 2025 saker <sakertooth@gmail.com>
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
//! Searching occurs on a background thread, and results are emitted as a Qt slot back to the user.
class FileSearch : public QObject
{
	Q_OBJECT
public:
	//! Represents a search task to be carried out by the search object.
	struct Task
	{
		QString filter;					 //! The filter to be tokenized.
		QStringList paths;				 //! The list of paths to search recursively through.
		QStringList extensions;			 //! The list of allowed extensions.
		QFlags<QDir::Filter> dirFilters; //! The directory filter flag.
	};

	//! Create a search object with the given @p parent (if any).
	FileSearch(QObject* parent = nullptr);

	//! Stop processing and destroys the object.
	~FileSearch();

	//! Cannot be copied.
	FileSearch(const FileSearch&) = delete;

	//! Cannot be moved.
	FileSearch(FileSearch&&) = delete;

	//! Cannot be copied.
	FileSearch& operator=(const FileSearch&) = delete;

	//! Cannot be moved.
	FileSearch& operator=(FileSearch&&) = delete;

	//! Commit to searching with the given @p task.
	//! Cancels any previous search.
	//! Callers can connect to the provided signals to interact with the search and its progress.
	void search(Task task);

signals:
	//! Emitted when the search object has found a matching path.
	void foundMatch(const QString& path);

	//! Emitted when the search object has started searching.
	void started();

	//! Emitted when the search object has finished searching.
	void finished();

private:
	void runSearch(Task task);
	std::future<void> m_task;
	std::atomic_flag m_stop = ATOMIC_FLAG_INIT;
};
} // namespace lmms::gui

#endif // LMMS_GUI_FILE_SEARCH_H
