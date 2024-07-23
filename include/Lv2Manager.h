/*
 * Lv2Manager.h - Implementation of Lv2Manager class
 *
 * Copyright (c) 2018-2024 Johannes Lorenz <jlsf2013$users.sourceforge.net, $=@>
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

#ifndef LMMS_LV2_MANAGER_H
#define LMMS_LV2_MANAGER_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_LV2

#include <map>
#include <set>
#include <string_view>
#include <lilv/lilv.h>

#include "Lv2Basics.h"
#include "Lv2UridCache.h"
#include "Lv2UridMap.h"
#include "Plugin.h"


namespace lmms
{

/*
	all Lv2 classes in relation (use our "4 spaces per tab rule" to view):

	explanation:
		"x = {y z}" means class "x" consists of classes "y" and "z"
			(and probably other classes not mentioned)
		"x = {y*}" means class "x" references/uses class "y"

	core:
		Lv2Proc =			{LilvInstance}
		Lv2ControlBase =	{Lv2Proc, Lv2Proc... (2 for mono, 1 for stereo)}
		Lv2Manager =		{LilvPlugin*, LilvPlugin* ...}
							(creates Lv2ControlBase, Lv2ControlBase...)

		Lv2FxControls =		{Lv2ControlBase}
		Lv2Effect =			{Effect + Lv2FxControls}
							(takes Lv2SubPluginFeatures in ctor)
		Lv2Instrument =		{Instrument + Lv2ControlBase}
							(takes Lv2SubPluginFeatures in ctor)

	gui:
		Lv2ViewProc	=			{Lv2Proc*}
		Lv2ViewBase =			{Lv2ViewProc, Lv2ViewProc...
								 (2 for mono, 1 for stereo)}
		Lv2FxControlDialog =	{EffectControlDialog + Lv2ViewBase}
		Lv2InsView =			{InstrumentView + Lv2ViewBase}

	Lv2SubPluginFeatures:
		Lv2SubPluginFeatures =		{Lv2Manager*}
		Lv2Effect::Descriptor =		{Lv2SubPluginFeatures}
		Lv2Instrument::Descriptor =	{Lv2SubPluginFeatures}
*/


//! Class to keep track of all LV2 plugins
class Lv2Manager
{
public:
	void initPlugins();

	Lv2Manager();
	~Lv2Manager();


	AutoLilvNode uri(const char* uriStr);

	//! Class representing info for one plugin
	struct Lv2Info
	{
	public:
		//! use only for std::map internals
		Lv2Info() : m_plugin(nullptr) {}
		//! ctor used inside Lv2Manager
		Lv2Info(const LilvPlugin* plug, Plugin::Type type, bool valid) :
			m_plugin(plug), m_type(type), m_valid(valid) {}
		Lv2Info(Lv2Info&& other) = default;
		Lv2Info& operator=(Lv2Info&& other) = default;

		const LilvPlugin* plugin() const { return m_plugin; }
		Plugin::Type type() const { return m_type; }
		bool isValid() const { return m_valid; }

	private:
		const LilvPlugin* m_plugin;
		Plugin::Type m_type;
		bool m_valid = false;
	};

	//! Return descriptor with URI @p uri or nullptr if none exists
	const LilvPlugin *getPlugin(const std::string &uri);
	//! Return descriptor with URI @p uri or nullptr if none exists
	const LilvPlugin *getPlugin(const QString& uri);

	using Lv2InfoMap = std::map<std::string, Lv2Info>;
	using Iterator = Lv2InfoMap::iterator;
	Iterator begin() { return m_lv2InfoMap.begin(); }
	Iterator end() { return m_lv2InfoMap.end(); }

	UridMap& uridMap() { return m_uridMap; }
	const Lv2UridCache& uridCache() const { return m_uridCache; }
	const std::set<std::string_view>& supportedFeatureURIs() const
	{
		return m_supportedFeatureURIs;
	}
	bool isFeatureSupported(const char* featName) const;
	AutoLilvNodes findNodes(const LilvNode *subject,
		const LilvNode *predicate, const LilvNode *object);

	static bool pluginIsUnstable(const char* pluginUri)
	{
		return unstablePlugins.find(pluginUri) != unstablePlugins.end();
	}
	static bool pluginIsOnlyUsefulWithUi(const char* pluginUri)
	{
		return pluginsOnlyUsefulWithUi.find(pluginUri) != pluginsOnlyUsefulWithUi.end();
	}
	static bool pluginIsUnstableWithBuffersizeLessEqual32(const char* pluginUri)
	{
		return unstablePluginsBuffersizeLessEqual32.find(pluginUri) !=
			unstablePluginsBuffersizeLessEqual32.end();
	}

	//! Whether the user generally wants a UI (and we generally support that)
	//! Since we do not generally support UI right now, this will always return false...
	static bool wantUi();

private:
	// general data
	bool m_debug; //!< if set, debug output will be printed
	LilvWorld* m_world;
	Lv2InfoMap m_lv2InfoMap;
	std::set<std::string_view> m_supportedFeatureURIs;

	// feature data that are common for all Lv2Proc
	UridMap m_uridMap;

	// URID cache for fast URID access
	Lv2UridCache m_uridCache;

	// static
	static const std::set<std::string_view> unstablePlugins;
	static const std::set<std::string_view> pluginsOnlyUsefulWithUi;
	static const std::set<std::string_view> unstablePluginsBuffersizeLessEqual32;

	// functions
	bool isSubclassOf(const LilvPluginClass *clvss, const char *uriStr);
};


} // namespace lmms

#endif // LMMS_HAVE_LV2

#endif // LMMS_LV2_MANAGER_H
