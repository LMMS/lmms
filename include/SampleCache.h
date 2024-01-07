/*
 * SampleCache.h - SampleBuffer cache
 *
 * Copyright (c) 2024 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#ifndef LMMS_SAMPLE_CACHE_H
#define LMMS_SAMPLE_CACHE_H

#include <QFileSystemWatcher>
#include <QHash>
#include <QString>
#include <chrono>
#include <unordered_map>
#include <map>

#include "NoCopyNoMove.h"
#include "SampleBuffer.h"
#include "lmms_export.h"

namespace lmms {
class LMMS_EXPORT SampleCache : public QObject
{
	Q_OBJECT
public:
	SampleCache();
	~SampleCache() = default;

	static auto createBufferFromFile(const QString& filePath,
		std::chrono::seconds keepAlive = std::chrono::seconds{0}) -> std::shared_ptr<const SampleBuffer>;

	static auto createBufferFromBase64(const QString& base64,
		int sampleRate = Engine::audioEngine()->processingSampleRate(),
		std::chrono::seconds keepAlive = std::chrono::seconds{0}) -> std::shared_ptr<const SampleBuffer>;

	static void add(const SampleBuffer& buffer);
	static auto remove(const SampleBuffer& buffer) -> bool;

	static auto get(const QString& source, SampleBuffer::Source sourceType) -> std::shared_ptr<const SampleBuffer>;

	static auto replace(const SampleBuffer& buffer) -> bool;

public slots:
	void removeFile(const QString& path);

private:
	static std::map<std::pair<QString, SampleBuffer::Source>, std::weak_ptr<const SampleBuffer>> s_entries;
	static QFileSystemWatcher s_watcher;
};

class KeepAlive : public QObject, public NoCopyNoMove
{
	Q_OBJECT
public:
	KeepAlive(QObject* parent, std::shared_ptr<const SampleBuffer>&& p)
		: QObject{parent}
		, m_data{std::move(p)}
	{
	}

public slots:
	void destroy()
	{
		m_data.reset();
		delete this;
	}
private:
	std::shared_ptr<const SampleBuffer> m_data;
};

} // namespace lmms

#endif // LMMS_SAMPLE_CACHE_H
