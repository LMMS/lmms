/*
 * SampleLoader.h - Sample loader with support for caching
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
#include <unordered_map>

#include "SampleBuffer.h"
#include "lmms_export.h"

namespace lmms {

class LMMS_EXPORT SampleLoader : public QObject
{
	Q_OBJECT
public:
	~SampleLoader() override = default;

	static auto instance() -> SampleLoader&;

	static auto fromFile(const QString& filePath, bool cache = true)
		-> std::shared_ptr<const SampleBuffer>;

	static auto fromBase64(const QString& base64, int sampleRate, bool cache = true)
		-> std::shared_ptr<const SampleBuffer>;

	static auto fromBase64(const QString& base64, bool cache = true)
		-> std::shared_ptr<const SampleBuffer>;

private:
	SampleLoader();

	struct AutoEvictor;

	auto get(const SampleBuffer::Source& source) -> std::shared_ptr<const SampleBuffer>;

	void add(const std::shared_ptr<const SampleBuffer>& buffer);
	auto remove(const SampleBuffer& buffer) -> bool;

	using Key = SampleBuffer::Source;
	using Value = std::weak_ptr<const SampleBuffer>;

	std::unordered_map<Key, Value, Key::Hasher> m_entries;
	QFileSystemWatcher m_watcher;
};

} // namespace lmms

#endif // LMMS_SAMPLE_LOADER_H
