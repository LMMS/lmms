/*
 * ClapGui.cpp - Implements CLAP GUI extension
 *
 * Copyright (c) 2023 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#include "ClapGui.h"
#include "ClapInstance.h"
#include "WindowEmbed.h"

#include <QApplication>
#include <QGuiApplication>
#include <QWindow>

#include "GuiApplication.h"
#include "MainWindow.h"
#include "SubWindow.h"

#include <clap/plugin.h>

#include <algorithm>
#include <cassert>

namespace lmms
{

ClapGui::ClapGui(const ClapPluginInfo* info, const clap_plugin* plugin, const clap_plugin_gui* gui)
	: m_pluginInfo{info}
	, m_plugin{plugin}
	, m_gui{gui}
{
	assert(m_pluginInfo != nullptr);
	assert(m_plugin != nullptr);
	assert(extensionSupported(m_gui));
}

ClapGui::~ClapGui()
{
	destroy();
}

auto ClapGui::create() -> bool
{
	destroy();

	m_embedMethod = WindowEmbed::Method::Headless;
	const auto windowId = gui::getGUI()->mainWindow()->winId();
	auto window = clap_window{};
#if defined(LMMS_BUILD_WIN32)
	m_embedMethod = WindowEmbed::Method::Win32;
	window.api = CLAP_WINDOW_API_WIN32;
	window.win32 = reinterpret_cast<clap_hwnd>(windowId);
#elif defined(LMMS_BUILD_APPLE)
	m_embedMethod = WindowEmbed::Method::Cocoa;
	window.api = CLAP_WINDOW_API_COCOA;
	window.cocoa = reinterpret_cast<clap_nsview>(windowId);
#elif defined(LMMS_BUILD_LINUX)
	m_embedMethod = WindowEmbed::Method::XEmbed;
	window.api = CLAP_WINDOW_API_X11;
	window.x11 = windowId;
#else
	qDebug() << "CLAP GUI: Unsupported platform";
	return false;
#endif

	if (!m_gui->is_api_supported(m_plugin, window.api, false))
	{
		if (!m_gui->is_api_supported(m_plugin, window.api, true))
		{
			qDebug() << "CLAP GUI: Plugin does not support gui api";
			return false;
		}
		m_embedMethod = WindowEmbed::Method::None; // floating
	}

	if (!m_gui->create(m_plugin, window.api, isFloating()))
	{
		qWarning() << "CLAP GUI: Could not create the plugin gui";
		return false;
	}

	m_guiCreated = true;
	assert(m_guiVisible == false);

	if (isFloating())
	{
		m_gui->set_transient(m_plugin, &window);
		if (const auto name = m_pluginInfo->descriptor()->name; name && name[0] != '\0')
		{
			m_gui->suggest_title(m_plugin, name);
		}
	}
	else
	{
		std::uint32_t width = 0;
		std::uint32_t height = 0;

		if (!m_gui->get_size(m_plugin, &width, &height))
		{
			qWarning() << "CLAP GUI: Could not get the size of the plugin gui";
			m_guiCreated = false;
			m_gui->destroy(m_plugin);
			return false;
		}

		//mainWindow()->resizePluginView(width, height);

		if (!m_gui->set_parent(m_plugin, &window))
		{
			qWarning() << "CLAP GUI: Could not embed the plugin gui";
			m_guiCreated = false;
			m_gui->destroy(m_plugin);
			return false;
		}
	}

	return true;
}

void ClapGui::destroy()
{
	if (m_guiCreated)
	{
		m_gui->destroy(m_plugin);
		m_guiCreated = false;
		m_guiVisible = false;
	}
}

auto ClapGui::extensionSupported(const clap_plugin_gui* ext) noexcept -> bool
{
	return ext && ext->is_api_supported && ext->get_preferred_api && ext->create && ext->destroy
		&& ext->set_scale && ext->get_size && ext->show && ext->hide
		&& (windowSupported(ext, true) || windowSupported(ext, false));
}

auto ClapGui::windowSupported(const clap_plugin_gui* ext, bool floating) noexcept -> bool
{
	// NOTE: This method only checks if the needed API functions are implemented.
	//       Still need to call ext->is_api_supported
	assert(ext != nullptr);

	if (floating)
	{
		// Needed for floating windows
		return ext->set_transient && ext->suggest_title;
	}
	else
	{
		// Needed for embedded windows
		return ext->can_resize && ext->get_resize_hints && ext->adjust_size && ext->set_size && ext->set_parent;
	}
}

void ClapGui::clapResizeHintsChanged()
{
	qDebug() << "ClapGui::clapResizeHintsChanged";
	// TODO
}

auto ClapGui::clapRequestResize(std::uint32_t width, std::uint32_t height) -> bool
{
	qDebug() << "ClapGui::clapRequestResize";
	// TODO

	return true;
}

auto ClapGui::clapRequestShow() -> bool
{
	qDebug() << "ClapGui::clapRequestShow";
	// TODO

	return true;
}

auto ClapGui::clapRequestHide() -> bool
{
	qDebug() << "ClapGui::clapRequestHide";
	// TODO

	return true;
}

void ClapGui::clapRequestClosed(bool wasDestroyed)
{
	qDebug() << "ClapGui::clapRequestClosed";
	if (wasDestroyed)
	{
		m_gui->destroy(m_plugin);
		m_guiCreated = false;
		m_guiVisible = false;
	}
}

} // namespace lmms
