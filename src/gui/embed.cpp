/*
 * embed.cpp - misc stuff for using embedded resources (linked into binary)
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "embed.h"

#include <QDebug>
#include <QDir>
#include <QImageReader>
#include <QPixmapCache>
#include <QResource>

namespace lmms::embed {

namespace {

auto loadPixmap(const QString& name, int width, int height, const char* const* xpm) -> QPixmap
{
	if (xpm) { return QPixmap{xpm}; }

	const auto resourceName = QDir::isAbsolutePath(name) ? name : "artwork:" + name;
	auto reader = QImageReader{resourceName};
	if (width > 0 && height > 0) { reader.setScaledSize(QSize{width, height}); }

	const auto pixmap = QPixmap::fromImageReader(&reader);
	if (pixmap.isNull()) {
		qWarning().nospace() << "Error loading icon pixmap " << name << ": " << reader.errorString();
		return QPixmap{1, 1};
	}
	return pixmap;
}

} // namespace

auto getIconPixmap(std::string_view name, int width, int height, const char* const* xpm) -> QPixmap
{
	if (name.empty()) { return QPixmap{}; }

	const auto pixmapName = QString::fromUtf8(name.data(), name.size());
	const auto cacheName = (width > 0 && height > 0)
		? QStringLiteral("%1_%2_%3").arg(pixmapName, width, height)
		: pixmapName;

	// Return cached pixmap if it exists
	if (auto pixmap = QPixmap{}; QPixmapCache::find(cacheName, &pixmap)) { return pixmap; }

	// Load the pixmap and cache it before returning
	const auto pixmap = loadPixmap(pixmapName, width, height, xpm);
	QPixmapCache::insert(cacheName, pixmap);
	return pixmap;
}

auto getText(std::string_view name) -> QString
{
	const auto resource = QResource{":/" + QString::fromUtf8(name.data(), name.size())};
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
	return QString::fromUtf8(resource.uncompressedData());
#else
	return QString::fromUtf8(reinterpret_cast<const char*>(resource.data()), resource.size());
#endif
}

} // namespace lmms::embed
