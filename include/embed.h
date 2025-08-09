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

#include <QPixmap>
#include <QString>

#include "lmms_export.h"
#ifdef PLUGIN_NAME
#include "LmmsCommonMacros.h"
#endif

namespace lmms {

namespace embed {

/**
 * Return an image for the icon pixmap cache.
 *
 * @param name Identifier for the pixmap. If it is not in the icon pixmap
 *   cache, it will be loaded from the artwork QDir search paths (exceptions are
 *   compiled-in XPMs, you need to provide @p xpm for loading them).
 * @param xpm Must be XPM data if the source should be raw XPM data instead of
 *   a file
 */
auto LMMS_EXPORT getIconPixmap(std::string_view name,
	int width = -1, int height = -1, const char* const* xpm = nullptr) -> QPixmap;
auto LMMS_EXPORT getText(std::string_view name) -> QString;

/**
 * @brief Temporary shim for QPixmap::deviceIndependentSize.
 * @param pixmap The pixmap to get the size of.
 * @return The device-independent size of the pixmap.
 */
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
[[deprecated("Use QPixmap::deviceIndependentSize() instead; See "
             "https://doc.qt.io/qt-6/qpixmap.html#deviceIndependentSize")]]
#endif
inline auto logicalSize(const QPixmap &pixmap) noexcept
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
	return pixmap.deviceIndependentSize().toSize();
#else
	return pixmap.isNull() ? QSize() : pixmap.size() / pixmap.devicePixelRatio();
#endif
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

inline auto getIconPixmap(std::string_view name,
	int width = -1, int height = -1, const char* const* xpm = nullptr) -> QPixmap
{
	return PluginPixmapLoader{std::string{name}, xpm}.pixmap(width, height);
}

} // namespace PLUGIN_NAME

#endif // PLUGIN_NAME

} // namespace lmms

#endif // LMMS_EMBED_H
