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
#include "DpiContext.h"

#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QImageReader>
#include <QPainter>
#include <QPixmapCache>
#include <QResource>
#include <QScreen>
#include <QSvgRenderer>

namespace lmms::embed {

namespace {

// QPixmapCache and HiDPI compatible SVG-->QPixmap wrapper with explicit DPR
auto loadSvgPixmap(const QString& resourceName, int width, int height, qreal dpr) -> QPixmap
{
	// QFile requires the file extension to be present, unlike QImageReader
	QString fileName = resourceName;
	if (!fileName.endsWith(".svg", Qt::CaseInsensitive)) { fileName += ".svg"; }

	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly))
	{
		qWarning() << "Failed to open resource for SVG: " << resourceName;
		return QPixmap{1, 1};
	}

	QSvgRenderer renderer(file.readAll());
	if (!renderer.isValid())
	{
		qWarning() << "Error loading SVG file: " << resourceName;
		return QPixmap{1, 1};
	}

	// Get the default size of the SVG (without scaling)
	QSize logicalSize = renderer.defaultSize();

	// If width and/or height are provided, compute target logical size
	if (width > 0 && height > 0)
	{
		logicalSize = QSize(width, height);
	}
	else if (width > 0 && logicalSize.width() > 0)
	{
		logicalSize = QSize(width, (width * logicalSize.height()) / logicalSize.width());
	}
	else if (height > 0 && logicalSize.height() > 0)
	{
		logicalSize = QSize((height * logicalSize.width()) / logicalSize.height(), height);
	}

	const qreal effectiveDpr = dpr > 0.0 ? dpr : 1.0;
	const QSize physSize = gui::Dpi::rasterSizeFor(logicalSize, effectiveDpr);

	QImage image(physSize, QImage::Format_ARGB32_Premultiplied);
	image.fill(Qt::transparent);
	QPainter painter(&image);
	renderer.render(&painter, QRectF(QPointF(0, 0), QSizeF(physSize)));
	painter.end();

	auto pixmap = QPixmap::fromImage(image);
	pixmap.setDevicePixelRatio(effectiveDpr);

	return pixmap;
}

auto loadPixmap(const QString& name, int width, int height, qreal dpr, const char* const* xpm) -> QPixmap
{
	const qreal effectiveDpr = dpr > 0.0 ? dpr : 1.0;

	if (xpm)
	{
		QPixmap pixmap{xpm};
		if (width > 0 && height > 0)
		{
			const QSize physSize = gui::Dpi::rasterSizeFor(QSize(width, height), effectiveDpr);
			pixmap = pixmap.scaled(physSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
			pixmap.setDevicePixelRatio(effectiveDpr);
		}
		return pixmap;
	}

	const auto resourceName = QDir::isAbsolutePath(name) ? name : "artwork:" + name;

	if (resourceName.endsWith(".svg", Qt::CaseInsensitive))
	{
		return loadSvgPixmap(resourceName, width, height, effectiveDpr);
	}

	auto reader = QImageReader{resourceName};

	if (reader.format().toLower() == "svg")
	{
		return loadSvgPixmap(resourceName, width, height, effectiveDpr);
	}

	// Check for @2x high-resolution asset variant when scaling factor warrants it
	if (effectiveDpr > 1.2)
	{
		QString name2x = resourceName;
		const int dotIdx = name2x.lastIndexOf('.');
		if (dotIdx > 0)
		{
			name2x.insert(dotIdx, "@2x");
		}
		else
		{
			name2x += "@2x";
		}

		if (QFile::exists(name2x) || QResource(name2x).isValid())
		{
			QImageReader reader2x(name2x);
			if (width > 0 && height > 0)
			{
				const QSize physSize = gui::Dpi::rasterSizeFor(QSize(width, height), effectiveDpr);
				reader2x.setScaledSize(physSize);
			}
			auto pm2x = QPixmap::fromImageReader(&reader2x);
			if (!pm2x.isNull())
			{
				pm2x.setDevicePixelRatio(effectiveDpr);
				return pm2x;
			}
		}
	}

	if (width > 0 && height > 0)
	{
		const QSize physSize = gui::Dpi::rasterSizeFor(QSize(width, height), effectiveDpr);
		reader.setScaledSize(physSize);
	}

	auto pixmap = QPixmap::fromImageReader(&reader);
	if (pixmap.isNull())
	{
		qWarning().nospace() << "Error loading icon pixmap " << name << ": " << reader.errorString();
		return QPixmap{1, 1};
	}

	if (width > 0 && height > 0)
	{
		pixmap.setDevicePixelRatio(effectiveDpr);
	}
	return pixmap;
}

} // namespace

auto getIconPixmap(std::string_view name, qreal dpr, int width, int height, const char* const* xpm) -> QPixmap
{
	if (name.empty()) { return QPixmap{}; }

	const auto pixmapName = QString::fromUtf8(name.data(), name.size());
	const qreal effectiveDpr = dpr > 0.0 ? dpr : 1.0;

	const auto cacheName = QStringLiteral("%1_w%2_h%3_dpr%4")
		.arg(pixmapName)
		.arg(width)
		.arg(height)
		.arg(QString::number(effectiveDpr, 'f', 2));

	// Return cached pixmap if it exists
	if (auto pixmap = QPixmap{}; QPixmapCache::find(cacheName, &pixmap))
	{
		return pixmap;
	}

	// Load the pixmap and cache it before returning
	const auto pixmap = loadPixmap(pixmapName, width, height, effectiveDpr, xpm);
	QPixmapCache::insert(cacheName, pixmap);
	return pixmap;
}

auto getIconPixmap(std::string_view name, int width, int height, const char* const* xpm) -> QPixmap
{
	return getIconPixmap(name, 1.0, width, height, xpm);
}

auto getIconPixmap(std::string_view name, const PixmapRequest& request, const char* const* xpm) -> QPixmap
{
	return getIconPixmap(name, request.devicePixelRatio, request.logicalSize.width(), request.logicalSize.height(), xpm);
}

auto getText(std::string_view name) -> QString
{
	const auto resource = QResource{":/" + QString::fromUtf8(name.data(), name.size())};
	if (!resource.isValid())
	{
		qWarning().nospace() << "Error loading text resource '"
			<< QString::fromUtf8(name.data(), name.size()) << "'";
		return {};
	}
	const auto data = resource.uncompressedData();
	if (data.isNull())
	{
		qWarning().nospace() << "Error reading text resource '"
			<< QString::fromUtf8(name.data(), name.size()) << "'";
		return {};
	}
	return QString::fromUtf8(data);
}

} // namespace lmms::embed
