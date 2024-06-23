/*
 * FileSearch.h - File system search task
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

#ifndef LMMS_FILE_SEARCH_H
#define LMMS_FILE_SEARCH_H

#include <QDir>
#include <QObject>
#include <atomic>

namespace lmms {
//! A Qt object that encapsulates the operation of searching the file system.
class FileSearch : public QObject
{
	Q_OBJECT
public:
	//! Number of milliseconds the search waits before signaling a matching result.
	static constexpr int MillisecondsBetweenResults = 1;

	//! Create a `FileSearch` object that uses the specified string filter `filter` and extension filters in
	//! `extensions` to search within the given `paths`.
	//! `excludedPaths`, `dirFilters`, and `sortFlags` can optionally be specified to exclude certain directories, filter
	//! out certain types of entries, and sort the matches.
	FileSearch(const QString& filter, const QStringList& paths, const QStringList& extensions,
		const QStringList& excludedPaths = {}, QDir::Filters dirFilters = QDir::Filters{},
		QDir::SortFlags sortFlags = QDir::SortFlags{});

	//! Execute the search, emitting the `foundResult` signal when matches are found.
	void operator()();

	//! Cancel the search.
	void cancel();

signals:
	//! Emitted when a result is found when searching the file system.
	void foundMatch(FileSearch* search, const QString& match);

	//! Emitted when the search completes.
	void searchCompleted(FileSearch* search);

private:
	static auto isPathExcluded(const QString& path) -> bool;
	QString m_filter;
	QStringList m_paths;
	QStringList m_extensions;
	QStringList m_excludedPaths;
	QDir::Filters m_dirFilters;
	QDir::SortFlags m_sortFlags;
	std::atomic<bool> m_cancel = false;
};
} // namespace lmms
#endif // LMMS_FILE_SEARCH_H
