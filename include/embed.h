/*
 * embed.h - misc. stuff for using embedded data (resources linked into binary)
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

#ifndef LMMS_EMBED_H
#define LMMS_EMBED_H

#include <string>
#include <string_view>

#include <QIcon>
#include <QImage>
#include <QPixmap>
#include <QString>
#include <QSize>
#include <QSizeF>

#include "lmms_export.h"
#ifdef PLUGIN_NAME
#include "LmmsCommonMacros.h"
#endif

namespace lmms {

namespace embed {

struct PixmapRequest
{
	QSize logicalSize = QSize(-1, -1);
	qreal devicePixelRatio = 1.0;
};

//! @brief Return an image for the icon pixmap cache with default DPR.
//! @param name Identifier for the pixmap. If it is not in the icon pixmap cache, it will be loaded from the artwork
//! QDir search paths (exceptions are compiled-in XPMs, you need to provide @p xpm for loading them).
//! @param width A specific pixmap width. When this and @p height are provided, the pixmap is cached.
//! @param height A specific pixmap height. When this and @p width are provided, the pixmap is cached.
//! @param xpm Must be XPM data if the source should be raw XPM data instead of a file
auto LMMS_EXPORT getIconPixmap(
	std::string_view name,
	int width = -1,
	int height = -1,
	const char* const* xpm = nullptr
) -> QPixmap;

//! @brief Return an image for the icon pixmap cache with explicit device pixel ratio.
auto LMMS_EXPORT getIconPixmap(
	std::string_view name,
	qreal dpr,
	int width = -1,
	int height = -1,
	const char* const* xpm = nullptr
) -> QPixmap;

//! @brief Return an image for the icon pixmap cache using a PixmapRequest.
auto LMMS_EXPORT getIconPixmap(
	std::string_view name,
	const PixmapRequest& request,
	const char* const* xpm = nullptr
) -> QPixmap;

//! Native file-backed icons let Qt request the appropriate raster for menus/toolbars/windows.
auto LMMS_EXPORT getIcon(std::string_view name, const char* const* xpm = nullptr) -> QIcon;
auto LMMS_EXPORT iconForPixmap(const QPixmap& pixmap) -> QIcon;
auto LMMS_EXPORT getText(std::string_view name) -> QString;

// A resource retains its requested logical size independently of ceil-rounded raster dimensions.
// Keep this descriptor in widgets that must reload a resource after a monitor change.
struct LMMS_EXPORT PixmapResource
{
	QPixmap pixmap;
	QSize logicalSize = QSize(0, 0);
	std::string name;
	QImage rasterSource; // Own XPM pixels; callers need not keep their input strings alive.

	auto atDpr(qreal dpr) const -> PixmapResource;
};

auto LMMS_EXPORT getIconResource(std::string_view name, const PixmapRequest& request = {},
	const char* const* xpm = nullptr) -> PixmapResource;
auto LMMS_EXPORT describePixmap(const QPixmap& pixmap) -> PixmapResource;
auto LMMS_EXPORT logicalSize(const QPixmap& pixmap) -> QSize;

inline auto logicalSize(const QImage& image) noexcept -> QSize
{
	return image.isNull() ? QSize(0, 0) : (QSizeF(image.size()) / image.devicePixelRatio()).toSize();
}

} // namespace embed

class PixmapLoader
{
public:
	PixmapLoader() = default;

	explicit PixmapLoader(std::string name, const char* const* xpm = nullptr) :
		m_name{std::move(name)},
		m_xpm{xpm}
	{ }

	virtual ~PixmapLoader() = default;

	auto pixmap(int width = -1, int height = -1) const -> QPixmap
	{
		return embed::getIconPixmap(m_name, width, height, m_xpm);
	}

	auto pixmap(qreal dpr, int width = -1, int height = -1) const -> QPixmap
	{
		return embed::getIconPixmap(m_name, dpr, width, height, m_xpm);
	}

	auto icon() const -> QIcon { return embed::getIcon(m_name, m_xpm); }
	auto pixmapName() const -> const std::string& { return m_name; }

private:
	std::string m_name;
	const char* const* m_xpm = nullptr;
};

#ifdef PLUGIN_NAME

class PluginPixmapLoader : public PixmapLoader
{
public:
	PluginPixmapLoader() = default;

	explicit PluginPixmapLoader(std::string name, const char* const* xpm = nullptr) :
		PixmapLoader{LMMS_STRINGIFY(PLUGIN_NAME) "/" + name, xpm}
	{ }
};

namespace PLUGIN_NAME {

inline auto getIcon(std::string_view name) -> QIcon
{
	return PluginPixmapLoader{std::string{name}}.icon();
}

inline auto getIconPixmap(std::string_view name,
	int width = -1, int height = -1, const char* const* xpm = nullptr) -> QPixmap
{
	return PluginPixmapLoader{std::string{name}, xpm}.pixmap(width, height);
}

inline auto getIconPixmap(std::string_view name,
	qreal dpr, int width = -1, int height = -1, const char* const* xpm = nullptr) -> QPixmap
{
	return PluginPixmapLoader{std::string{name}, xpm}.pixmap(dpr, width, height);
}

} // namespace PLUGIN_NAME

#endif // PLUGIN_NAME

} // namespace lmms

#endif // LMMS_EMBED_H
