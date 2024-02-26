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
	FileSearch(const QString& filter, const QStringList& paths, const QStringList& extensions);

	//! Execute the search, emitting the `foundResult` signal when results are found.
	void operator()();

	//! Cancel the search.
	void cancel();

signals:
	//! Emitted when a result is found when searching the file system.
	void foundResult(FileSearch* search, QString result);

private:
	static auto pathInBlacklist(const QString& path) -> bool;
	static auto dirFilters() -> QDir::Filters;
	static auto sortFlags() -> QDir::SortFlags;
	QString m_filter;
	QStringList m_paths;
	QStringList m_extensions;
	std::atomic<bool> m_cancel = false;
};
} // namespace lmms
#endif // LMMS_FILE_SEARCH_H