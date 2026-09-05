/*
 * FileSearchJob.cpp
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

#include "FileSearchJob.h"

#include <QDirIterator>
#include <QRegularExpression>

#include "ThreadPool.h"

namespace lmms::gui {
FileSearchJob::FileSearchJob(QObject* parent)
	: QObject(parent)
{
}

FileSearchJob::~FileSearchJob()
{
	if (m_task.valid())
	{
		m_stop.test_and_set(std::memory_order_acquire);
		m_task.get();
	}
}

void FileSearchJob::search(Task task)
{
	if (m_task.valid())
	{
		m_stop.test_and_set(std::memory_order_acquire);
		m_task.get();
		m_stop.clear(std::memory_order_release);
	}

	const auto fn = [this, task = std::move(task)] { runSearch(std::move(task)); };
	m_task = ThreadPool::instance().enqueue(std::move(fn));
}

void FileSearchJob::runSearch(Task task)
{
	// RE expression that matches regular tokens, and tokens with double quotes around them
	static auto s_tokenRe = QRegularExpression{R"(\"([^"]+)\"|(\S+))"};

	auto tokensIt = s_tokenRe.globalMatch(task.filter);
	auto tokens = QStringList{};

	while (tokensIt.hasNext())
	{
		const auto match = tokensIt.next();
		const auto quoted = match.captured(1);
		const auto plain = match.captured(2);

		if (!quoted.isEmpty()) { tokens.push_back(quoted); }
		if (!plain.isEmpty()) { tokens.push_back(plain); }
	}

	emit started();

	for (const auto& path : task.paths)
	{
		auto dirIt = QDirIterator{path, task.dirFilters,
			QDirIterator::IteratorFlag::Subdirectories | QDirIterator::IteratorFlag::FollowSymlinks};

		while (dirIt.hasNext() && !m_stop.test(std::memory_order_relaxed))
		{
			const auto fileInfo = QFileInfo{dirIt.next()};
			const auto fileName = fileInfo.fileName();
			const auto containsToken = std::all_of(tokens.begin(), tokens.end(),
				[&](const auto& token) { return fileName.contains(token, Qt::CaseInsensitive); });

			const auto validDir = fileInfo.isDir() && containsToken;
			const auto validFile = fileInfo.isFile() && containsToken
				&& task.extensions.contains(QString{"*.%1"}.arg(fileInfo.completeSuffix()), Qt::CaseInsensitive);

			if (validDir || validFile) { emit foundMatch(fileInfo.filePath()); }
		}
	}

	emit finished();
}

} // namespace lmms::gui
