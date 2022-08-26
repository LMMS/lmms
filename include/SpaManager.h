/*
 * SpaManager.h - Implementation of SpaManager class
 *
 * Copyright (c) 2018-2019 Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>
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

#ifndef SPAMANAGER_H
#define SPAMANAGER_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_SPA

#include <map>
#include <spa/spa_fwd.h>

#include "Plugin.h"

class QLibrary;

namespace lmms
{

//! Class to keep track of all SPA plugins
class SpaManager
{
	Plugin::PluginTypes computePluginType(spa::descriptor *desc);

public:
	struct SpaInfo
	{
		// only required when plugins shall not be loaded at startup
		/*const*/ QString m_path;
		QLibrary *m_lib = nullptr;
		spa::descriptor *m_descriptor;
		Plugin::PluginTypes m_type;
		SpaInfo(const SpaInfo &) = delete;
		SpaInfo() = default;
		void cleanup();
	};

	SpaManager();
	~SpaManager();

	//! returns a descriptor with @p uniqueName or nullptr if none exists
	//! @param uniqueName The spa::unique_name of the plugin
	spa::descriptor *getDescriptor(const std::string &uniqueName);
	spa::descriptor *getDescriptor(const QString uniqueName);

	struct Iterator
	{
		std::map<std::string, SpaInfo>::iterator itr;
		bool operator!=(const Iterator &other)
		{
			return itr != other.itr;
		}
		Iterator &operator++()
		{
			++itr;
			return *this;
		}
		std::pair<const std::string, SpaInfo> &operator*()
		{
			return *itr;
		}
	};

	Iterator begin() { return {m_spaInfoMap.begin()}; }
	Iterator end() { return {m_spaInfoMap.end()}; }

private:
	std::map<std::string, SpaInfo> m_spaInfoMap;
};

} // namespace lmms

#endif // LMMS_HAVE_SPA

#endif // SPAMANAGER_H
