/*
 * CachedSampleLoader.h - Sample loader with a cache
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

#ifndef LMMS_CACHED_SAMPLE_LOADER_H
#define LMMS_CACHED_SAMPLE_LOADER_H

#include <QFileSystemWatcher>
#include <QObject>
#include <QString>
#include <map>

#include "SampleBuffer.h"
#include "SampleLoader.h"
#include "lmms_export.h"

namespace lmms {
class LMMS_EXPORT CachedSampleLoader : public QObject, public SampleLoader
{
	Q_OBJECT
public:
	~CachedSampleLoader() = default;

	static auto inst() -> CachedSampleLoader&;

	static auto createBufferFromFile(const QString& filePath) -> std::shared_ptr<const SampleBuffer>;

	static auto createBufferFromBase64(const QString& base64,
		int sampleRate = Engine::audioEngine()->processingSampleRate())
		-> std::shared_ptr<const SampleBuffer>;


private slots:
	void removeFile(const QString& path);

private:
	CachedSampleLoader();

	class AutoEvictor;

	void add(const std::shared_ptr<const SampleBuffer>& buffer);
	auto remove(const SampleBuffer& buffer) -> bool;

	auto get(const QString& source, SampleBuffer::Source sourceType) -> std::shared_ptr<const SampleBuffer>;

	std::map<std::pair<QString, SampleBuffer::Source>, std::weak_ptr<const SampleBuffer>> m_entries;
	QFileSystemWatcher m_watcher;
};

} // namespace lmms

#endif // LMMS_CACHED_SAMPLE_LOADER_H
