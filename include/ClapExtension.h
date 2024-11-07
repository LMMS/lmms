/*
 * ClapExtension.h - Base class templates for implementing CLAP extensions
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

#ifndef LMMS_CLAP_EXTENSION_H
#define LMMS_CLAP_EXTENSION_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_CLAP

#include <string_view>
#include <clap/plugin.h>

#include "lmms_export.h"
#include "NoCopyNoMove.h"

namespace lmms
{

class ClapInstance;
class ClapLog;

namespace detail
{

class LMMS_EXPORT ClapExtensionHelper : public NoCopyNoMove
{
public:
	ClapExtensionHelper(ClapInstance* instance)
		: m_instance{instance}
	{
	}

	virtual auto extensionId() const -> std::string_view = 0;
	virtual auto extensionIdCompat() const -> std::string_view { return std::string_view{}; }

	void beginPluginInit() { m_inPluginInit = true; }
	void endPluginInit() { m_inPluginInit = false; }
	auto logger() const -> const ClapLog&;

	static auto fromHost(const clap_host* host) -> ClapInstance*;

protected:
	enum class State
	{
		Uninit,
		PartialInit,
		FullInit,
		Unsupported
	};

	auto instance() const { return m_instance; }
	auto instance() { return m_instance; }
	auto host() const -> const clap_host*;
	auto plugin() const -> const clap_plugin*;

	/**
	 * For additional initialization steps.
	 * - Only called if basic init was successful
	 * - By default, will not be called during plugin->init()
	 *     unless `delayInit()` returns false
	 * - supported() == true during this call,
	 *     and (if applicable) pluginExt() is non-null
	 */
	virtual auto initImpl() noexcept -> bool { return true; }

	/**
	 * For additional deinitialization steps.
	 */
	virtual void deinitImpl() noexcept {}

	/**
	 * Whether `initImpl()` is postponed until after plugin->init()
	 */
	virtual auto delayInit() const noexcept -> bool { return true; }

	//! Whether the plugin is calling into the host during plugin->init()
	auto inPluginInit() const { return m_inPluginInit; }

private:
	ClapInstance* m_instance = nullptr;
	bool m_inPluginInit = false;
};

} // namespace detail


/**
 * Template for extensions with both a host and plugin side
 */
template<class HostExt, class PluginExt = void>
class LMMS_EXPORT ClapExtension : public detail::ClapExtensionHelper
{
public:
	using detail::ClapExtensionHelper::ClapExtensionHelper;
	virtual ~ClapExtension() = default;

	auto init() -> bool
	{
		switch (m_state)
		{
			case State::Uninit:
			{
				auto ext = static_cast<const PluginExt*>(plugin()->get_extension(plugin(), extensionId().data()));
				if (!ext)
				{
					// Try using compatibility ID if it exists
					if (const auto compatId = extensionIdCompat(); !compatId.empty())
					{
						ext = static_cast<const PluginExt*>(plugin()->get_extension(plugin(), compatId.data()));
					}
				}

				if (!ext || !checkSupported(*ext))
				{
					m_pluginExt = nullptr;
					m_state = State::Unsupported;
					return false;
				}

				m_pluginExt = ext;
				m_state = State::PartialInit;
				if ((!delayInit() || !inPluginInit()) && !initImpl())
				{
					m_state = State::Unsupported;
					return false;
				}

				m_state = delayInit() ? State::PartialInit : State::FullInit;

				return true;
			}
			case State::PartialInit:
			{
				if (inPluginInit()) { return false; }
				if (!initImpl())
				{
					m_state = State::Unsupported;
					return false;
				}
				m_state = State::FullInit;
				return true;
			}
			case State::FullInit:    return true;
			case State::Unsupported: return false;
			default:                 return false;
		}
	}

	void deinit()
	{
		deinitImpl();
		m_pluginExt = nullptr;
		m_state = State::Uninit;
	}

	/**
	 * Returns whether plugin implements required interface
	 *   and passes any additional checks from initImpl().
	 * Do not use before init().
	 */
	auto supported() const
	{
		return m_state == State::PartialInit || m_state == State::FullInit;
	}

	/**
	 * Returns pointer to host extension.
	 * If called during plugin.init(), will lazily initialize the extension.
	 */
	auto hostExt() -> const HostExt*
	{
		init();
		return hostExtImpl();
	}

	//! Non-null after init() is called if plugin implements the needed interface
	auto pluginExt() const { return m_pluginExt; }

protected:
	virtual auto hostExtImpl() const -> const HostExt* = 0;

	/**
	 * Checks whether the plugin extension implements the required
	 *   API methods for use within LMMS. May not be all the methods.
	 */
	virtual auto checkSupported(const PluginExt& ext) -> bool = 0;

private:
	//const HostExt* m_hostExt = nullptr;
	const PluginExt* m_pluginExt = nullptr;

	using State = detail::ClapExtensionHelper::State;
	State m_state = State::Uninit;
};


/**
 * Template for host-only extensions
*/
template<class HostExt>
class LMMS_EXPORT ClapExtension<HostExt, void> : public detail::ClapExtensionHelper
{
public:
	using detail::ClapExtensionHelper::ClapExtensionHelper;
	virtual ~ClapExtension() = default;

	auto init() -> bool
	{
		switch (m_state)
		{
			case State::Uninit:
			{
				m_state = State::PartialInit;
				if ((!delayInit() || !inPluginInit()) && !initImpl())
				{
					m_state = State::Unsupported;
					return false;
				}

				m_state = delayInit() ? State::PartialInit : State::FullInit;
				return true;
			}
			case State::PartialInit:
			{
				if (inPluginInit()) { return false; }
				if (!initImpl())
				{
					m_state = State::Unsupported;
					return false;
				}
				m_state = State::FullInit;
				return true;
			}
			case State::FullInit:    return true;
			case State::Unsupported: return false;
			default:                 return false;
		}
	}

	void deinit()
	{
		deinitImpl();
		m_state = State::Uninit;
	}

	/**
	 * Returns whether initImpl() was successful.
	 * Do not use before init().
	*/
	auto supported() const
	{
		return m_state == State::PartialInit || m_state == State::FullInit;
	}

	/**
	 * Returns pointer to host extension.
	 * If called during plugin.init(), will lazily initialize the extension.
	 */
	auto hostExt() -> const HostExt*
	{
		init();
		return hostExtImpl();
	}

protected:
	virtual auto hostExtImpl() const -> const HostExt* = 0;

private:
	//const HostExt* m_hostExt = nullptr;

	using State = detail::ClapExtensionHelper::State;
	State m_state = State::Uninit;
};

} // namespace lmms

#endif // LMMS_HAVE_CLAP

#endif // LMMS_CLAP_EXTENSION_H
