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

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <QCache>
#include <QFileInfo>
#include <QThread>
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

// Bounded metadata and owned XPM sources (cost in KiB, minimum 1 per entry).
// Rendered pixels remain governed by QPixmapCache's own memory limit.
// Widgets retain their descriptor, so eviction cannot remove their reload information.
QCache<qint64, PixmapResource> resourceDescriptions(4096);

auto targetSize(QSize natural, QSize requested) -> QSize
{
	if (requested.width() > 0 && requested.height() > 0) { return requested; }
	if (natural.isEmpty()) { return QSize(0, 0); }
	if (requested.width() > 0)
	{
		const qreal height = qreal(requested.width()) * natural.height() / natural.width();
		if (height >= std::numeric_limits<int>::max()) { return QSize(0, 0); }
		return QSize(requested.width(), std::max(1, qRound(height)));
	}
	if (requested.height() > 0)
	{
		const qreal width = qreal(requested.height()) * natural.width() / natural.height();
		if (width >= std::numeric_limits<int>::max()) { return QSize(0, 0); }
		return QSize(std::max(1, qRound(width)), requested.height());
	}
	return natural;
}

auto resolveSource(const QString& name) -> QString
{
	// Resolve each theme completely before considering a lower-priority theme.
	QStringList roots;
	QString relative = name;
	if (QDir::isAbsolutePath(name) || name.startsWith(":/")) { roots << QString(); }
	else
	{
		const auto colon = name.indexOf(':');
		const auto prefix = colon < 0 ? QStringLiteral("artwork") : name.left(colon);
		relative = colon < 0 ? name : name.mid(colon + 1);
		roots = QDir::searchPaths(prefix);
	}
	auto formats = QImageReader::supportedImageFormats();
	if (!formats.contains("svg")) { formats.append("svg"); }
	for (const auto& root : roots)
	{
		const QString base = root.isEmpty() ? relative : root + '/' + relative;
		if (QFile::exists(base)) { return base; }
		for (const auto& format : formats)
		{
			const auto candidate = base + '.' + QString::fromLatin1(format);
			if (QFile::exists(candidate)) { return candidate; }
		}
	}
	return {};
}

auto resolutionVariant(const QString& source, qreal dpr) -> QString
{
	if (dpr > 1 && !source.isEmpty() && !source.endsWith(".svg", Qt::CaseInsensitive)
		&& !QFileInfo(source).completeBaseName().endsWith("@2x"))
	{
		const int dot = source.lastIndexOf('.');
		const QString vectorSource = source.left(dot) + ".svg";
		const QString highRes = source.left(dot) + "@2x" + source.mid(dot);
		if (QFile::exists(vectorSource)) { return vectorSource; }
		else
		{
			QImageReader highReader(highRes);
			if (highReader.canRead()) { return highRes; }
		}
	}
	return source;
}

struct IconSource
{
	QImage image;
	QSize natural;
	std::unique_ptr<QSvgRenderer> svg;
};

auto readIconSource(const QString& source, const QString& selectedSource, const char* const* xpm) -> IconSource
{
	IconSource data;
	if (xpm)
	{
		data.image = QImage(xpm);
		data.natural = data.image.size();
	}
	else if (!source.isEmpty())
	{
		QImageReader reader(source);
		if (selectedSource.endsWith(".svg", Qt::CaseInsensitive) || reader.format().toLower() == "svg")
		{
			data.svg = std::make_unique<QSvgRenderer>(selectedSource);
			if (data.svg->isValid()) { data.natural = data.svg->defaultSize(); }
		}
		else
		{
			data.natural = reader.size();
			// An explicitly named @2x source also retains its base logical dimensions.
			if (QFileInfo(source).completeBaseName().endsWith("@2x")) { data.natural /= 2; }
		}
	}
	return data;
}

auto rasterizeIcon(IconSource& data, const QString& selectedSource, const char* const* xpm,
	const QSize& raster, qreal dpr) -> QPixmap
{
	if (data.svg)
	{
		data.image = QImage(raster, QImage::Format_ARGB32_Premultiplied);
		data.image.setDevicePixelRatio(dpr);
		data.image.fill(Qt::transparent);
		QPainter painter(&data.image);
		// Fill the ceil-rounded raster. Consumers draw it into the separately retained
		// logical target, so rounding cannot introduce transparent layout margins.
		data.svg->render(&painter, QRectF(QPointF(), QSizeF(raster) / dpr));
	}
	else
	{
		if (!xpm) { data.image = QImage(selectedSource); }
		data.image = data.image.scaled(raster, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
		data.image.setDevicePixelRatio(dpr);
	}
	return QPixmap::fromImage(data.image);
}

auto restoreCachedResource(const QString& key, PixmapResource& result) -> bool
{
	if (QPixmapCache::find(key, &result.pixmap))
	{
		if (const auto* description = resourceDescriptions.object(result.pixmap.cacheKey()))
		{
			result.logicalSize = description->logicalSize;
			result.rasterSource = description->rasterSource;
			return true;
		}
	}
	return false;
}

void remember(const PixmapResource& resource)
{
	if (resource.pixmap.isNull()) { return; }
	auto* description = new PixmapResource(resource);
	// Do not keep raster storage alive through the metadata cache.
	description->pixmap = QPixmap();
	const qint64 sourceKiB = (resource.rasterSource.sizeInBytes() + 1023) / 1024;
	const int cost = static_cast<int>(std::min<qint64>(1 + sourceKiB, std::numeric_limits<int>::max()));
	resourceDescriptions.insert(resource.pixmap.cacheKey(), description, cost);
}

} // namespace

auto getIconResource(std::string_view name, const PixmapRequest& request, const char* const* xpm) -> PixmapResource
{
	Q_ASSERT(qApp && QThread::currentThread() == qApp->thread());
	PixmapResource result;
	result.name = std::string(name);
	if (name.empty()) { return result; }
	const qreal dpr = std::isfinite(request.devicePixelRatio) && request.devicePixelRatio > 0
		? request.devicePixelRatio : 1.0;
	const QString source = xpm ? QString() : resolveSource(QString::fromUtf8(name.data(), name.size()));
	const QString selectedSource = xpm ? "xpm:" + QString::fromUtf8(name.data(), name.size())
		: resolutionVariant(source, dpr);
	const auto cacheKeyFor = [&](QSize size) {
		return QStringLiteral("lmms:%1:%2:%3:%4:%5")
			.arg(selectedSource).arg(quintptr(xpm), 0, 16)
			.arg(size.width()).arg(size.height())
			.arg(QString::number(dpr, 'g', std::numeric_limits<qreal>::max_digits10));
	};
	const QString key = cacheKeyFor(request.logicalSize);
	if (restoreCachedResource(key, result)) { return result; }
	auto data = readIconSource(source, selectedSource, xpm);
	if (xpm) { result.rasterSource = data.image; }
	result.logicalSize = targetSize(data.natural, request.logicalSize);
	if (data.natural.isEmpty() || result.logicalSize.isEmpty())
	{
		qWarning() << "Cannot load icon resource:" << QString::fromUtf8(name.data(), name.size());
		result.logicalSize = QSize(1, 1);
		result.pixmap = QPixmap(1, 1);
		result.pixmap.fill(Qt::transparent);
		remember(result);
		return result;
	}
	const QSize raster = gui::Dpi::rasterSizeFor(result.logicalSize, dpr);
	if (raster.isEmpty()) { return result; }

	const QString canonicalKey = cacheKeyFor(result.logicalSize);
	if (!QPixmapCache::find(canonicalKey, &result.pixmap))
	{
		result.pixmap = rasterizeIcon(data, selectedSource, xpm, raster, dpr);
		QPixmapCache::insert(canonicalKey, result.pixmap);
	}
	if (key != canonicalKey) { QPixmapCache::insert(key, result.pixmap); }
	remember(result);
	return result;
}

auto describePixmap(const QPixmap& pixmap) -> PixmapResource
{
	if (const auto* description = resourceDescriptions.object(pixmap.cacheKey()))
	{
		auto resource = *description;
		resource.pixmap = pixmap;
		return resource;
	}
	return {pixmap, pixmap.isNull() ? QSize(0, 0) : (QSizeF(pixmap.size()) / pixmap.devicePixelRatio()).toSize(), {}, {}};
}

auto logicalSize(const QPixmap& pixmap) -> QSize
{
	return describePixmap(pixmap).logicalSize;
}

auto PixmapResource::atDpr(qreal dpr) const -> PixmapResource
{
	if (!rasterSource.isNull())
	{
		const qreal ratio = std::isfinite(dpr) && dpr > 0 ? dpr : 1.0;
		if (qFuzzyCompare(pixmap.devicePixelRatio(), ratio)) { return *this; }
		PixmapResource result = *this;
		const QString key = QStringLiteral("lmms:raster:%1:%2:%3:%4")
			.arg(rasterSource.cacheKey()).arg(logicalSize.width()).arg(logicalSize.height())
			.arg(QString::number(ratio, 'g', std::numeric_limits<qreal>::max_digits10));
		if (!QPixmapCache::find(key, &result.pixmap))
		{
			const QSize raster = gui::Dpi::rasterSizeFor(logicalSize, ratio);
			if (raster.isEmpty()) { result.pixmap = QPixmap(); return result; }
			auto image = rasterSource.scaled(raster, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
			image.setDevicePixelRatio(ratio);
			result.pixmap = QPixmap::fromImage(image);
			QPixmapCache::insert(key, result.pixmap);
		}
		remember(result);
		return result;
	}
	return name.empty() ? *this : getIconResource(name, {logicalSize, dpr});
}

auto getIconPixmap(std::string_view name, qreal dpr, int width, int height, const char* const* xpm) -> QPixmap
{
	return getIconResource(name, {QSize(width, height), dpr}, xpm).pixmap;
}

auto getIconPixmap(std::string_view name, int width, int height, const char* const* xpm) -> QPixmap
{
	// Context-free legacy requests are provisional 1x variants. Resource-aware consumers
	// retain the source and reload at their own window's DPR when shown.
	return getIconPixmap(name, 1.0, width, height, xpm);
}

auto getIconPixmap(std::string_view name, const PixmapRequest& request, const char* const* xpm) -> QPixmap
{
	return getIconResource(name, request, xpm).pixmap;
}

auto getIcon(std::string_view name, const char* const* xpm) -> QIcon
{
	if (name.empty()) { return {}; }
	if (xpm) { return QIcon(getIconPixmap(name, -1, -1, xpm)); }
	QString source = resolveSource(QString::fromUtf8(name.data(), name.size()));
	// Keep the selected theme, but let Qt rasterize a real vector source for each
	// icon request instead of preserving a single low-resolution pixmap.
	const int dot = source.lastIndexOf('.');
	if (dot >= 0)
	{
		const QString vector = source.left(dot) + ".svg";
		if (QFile::exists(vector)) { source = vector; }
	}
	return QIcon(source);
}

auto iconForPixmap(const QPixmap& pixmap) -> QIcon
{
	const auto source = describePixmap(pixmap);
	return source.name.empty() || !source.rasterSource.isNull() ? QIcon(pixmap) : getIcon(source.name);
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
