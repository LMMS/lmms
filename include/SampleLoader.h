/*
 * SampleLoader.h - Sample loader with an optional cache
 *
 * Copyright (c) 2023 saker <sakertooth@gmail.com>
 *               2024 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#ifndef LMMS_SAMPLE_LOADER_H
#define LMMS_SAMPLE_LOADER_H

#include <QFileSystemWatcher>
#include <QObject>
#include <QString>
#include <map>

#include "SampleBuffer.h"
#include "lmms_export.h"

namespace lmms {

class LMMS_EXPORT SampleLoader : public QObject
{
	Q_OBJECT
public:
	~SampleLoader() override = default;

	using Source = SampleBuffer::Source;

	enum class Cache
	{
		None,
		Read,
		ReadWrite
	};

	static auto inst() -> SampleLoader&;

	static auto fromFile(const QString& filePath, Cache cache = Cache::ReadWrite)
		-> std::shared_ptr<const SampleBuffer>;

	static auto fromBase64(const QString& base64, int sampleRate, Cache cache = Cache::ReadWrite)
		-> std::shared_ptr<const SampleBuffer>;

	static auto fromBase64(const QString& base64, Cache cache = Cache::ReadWrite)
		-> std::shared_ptr<const SampleBuffer>;

private slots:
	void removeFile(const QString& path);

private:
	SampleLoader();

	class AutoEvictor;

	void add(const std::shared_ptr<const SampleBuffer>& buffer);
	auto remove(const SampleBuffer& buffer) -> bool;

	auto getFile(const QString& filePath) -> std::shared_ptr<const SampleBuffer>;
	auto getBase64(const QString& base64) -> std::shared_ptr<const SampleBuffer>;

	using Key = std::pair<QString, Source>;
	using Value = std::weak_ptr<const SampleBuffer>;

	std::map<Key, Value> m_entries;
	QFileSystemWatcher m_watcher;
};

} // namespace lmms

#endif // LMMS_SAMPLE_LOADER_H
