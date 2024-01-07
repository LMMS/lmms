/*
 * SampleCache.h
 *
 * Copyright (c) 2024 saker <sakertooth@gmail.com>
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
#include <memory>

#include "lmms_export.h"

namespace lmms {
class SampleBuffer;

class LMMS_EXPORT SampleCache
{
public:
	SampleCache();

	auto get(const QString& path) -> std::shared_ptr<const SampleBuffer>;
	static auto instance() -> SampleCache&;

private:
	QHash<QString, std::weak_ptr<const SampleBuffer>> m_samples;
	QFileSystemWatcher m_fsWatcher;
};
} // namespace lmms

#endif // LMMS_SAMPLE_CACHE_H
