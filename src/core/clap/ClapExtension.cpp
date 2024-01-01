/*
 * ClapExtension.cpp - Base class template for implementing CLAP extensions
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

#include "ClapExtension.h"

#ifdef LMMS_HAVE_CLAP

#include "ClapInstance.h"
#include "ClapLog.h"

namespace lmms
{

auto detail::ClapExtensionHelper::logger() const -> const ClapLog*
{
	return &instance()->logger();
}

auto detail::ClapExtensionHelper::fromHost(const clap_host* host) -> ClapInstance*
{
	if (!host)
	{
		ClapLog::globalLog(CLAP_LOG_ERROR, "A plugin passed an invalid host pointer");
		return nullptr;
	}

	auto h = static_cast<ClapInstance*>(host->host_data);
	if (!h)
	{
		ClapLog::globalLog(CLAP_LOG_ERROR, "A plugin invalidated the host context pointer");
		return nullptr;
	}

	if (!h->plugin())
	{
		ClapLog::globalLog(CLAP_LOG_ERROR, "A plugin is calling the host API during factory.create_plugin(). "
			"It needs to wait for plugin.init() first.");
		return nullptr;
	}

	return h;
}

} // namespace lmms

#endif // LMMS_HAVE_CLAP
