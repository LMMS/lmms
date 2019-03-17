/*
 * Lv2Manager.h - Implementation of Lv2Manager class
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

#ifndef LV2MANAGER_H
#define LV2MANAGER_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_LV2

#include <map>
#include <lilv/lilv.h>

#include "Lv2Basics.h"
#include "Plugin.h"


/*
	all Lv2 classes in relation (use our "4 spaces per tab rule" to view):

	explanation:
		"x = {y z}" means class "x" consists of classes "y" and "z"
			(and probably other classes not mentioned)
		"x = {y*}" me}ns class "x" references/uses class "y"

	core:
		Lv2Proc =			{LilvInstance}
		Lv2ControlBase =	{Lv2Proc, Lv2Proc... (2 for mono, 1 for stereo)}
		Lv2Manager =		{LilvPlugin*, LilvPlugin* ...}
							(creates Lv2ControlBase, Lv2ControlBase...)

		Lv2FxControls =		{LilvControlBase}
		Lv2Effect =			{Effect + Lv2FxControls}
							(takes Lv2SubPluginFeatures in ctor)
		Lv2Instrument =		{Instrument + LilvControlBase}
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
	LilvWorld* m_world;

public:
	void initPlugins();

	Lv2Manager();
	~Lv2Manager();


	AutoLilvNode uri(const char* uriStr)
	{
		return AutoLilvNode(lilv_new_uri(m_world, uriStr));
	}

	//! Class representing info for one plugin
	struct Lv2Info
	{
	public:
		//! use only for std::map internals
		Lv2Info() : m_plugin(nullptr) {}
		//! ctor used inside Lv2Manager
		Lv2Info(const LilvPlugin* plug, Plugin::PluginTypes type, bool valid) :
			m_plugin(plug), m_type(type), m_valid(valid) {}
		Lv2Info(const Lv2Info &) = delete;
		Lv2Info(Lv2Info&& other) :
			m_plugin(other.m_plugin),
			m_type(std::move(other.m_type)),
			m_valid(std::move(other.m_valid))
		{
		}
		Lv2Info& operator=(Lv2Info&& other)
		{
			m_plugin = other.m_plugin;
			m_type = std::move(other.m_type);
			m_valid = std::move(other.m_valid);
			return *this;
		}
		bool isValid() const { return m_valid; }
		Plugin::PluginTypes type() const { return m_type; }
		const LilvPlugin* plugin() const { return m_plugin; }
	private:
		const LilvPlugin* m_plugin;
		Plugin::PluginTypes m_type;
		bool m_valid = false;
	};

	//! Return a descriptor with @p uniqueName or nullptr if none exists
	//! @param uniqueName The lv2::unique_name of the plugin
	const LilvPlugin *getPlugin(const std::string &uri);
	const LilvPlugin *getPlugin(const QString uri);

	class Iterator
	{
		std::map<std::string, Lv2Info>::iterator m_itr;
	public:
		bool operator!=(const Iterator &other)
		{
			return m_itr != other.m_itr;
		}
		Iterator &operator++()
		{
			++m_itr;
			return *this;
		}
		std::pair<const std::string, Lv2Info> &operator*()
		{
			return *m_itr;
		}
		Iterator(std::map<std::string, Lv2Info>::iterator itr) :
			m_itr(itr) {}
	};

	Iterator begin() { return Iterator(m_lv2InfoMap.begin()); }
	Iterator end() { return Iterator(m_lv2InfoMap.end()); }

private:
	std::map<std::string, Lv2Info> m_lv2InfoMap;
	bool isSubclassOf(const LilvPluginClass *clvss, const char *uriStr);
};

#endif // LMMS_HAVE_LV2

#endif // LV2MANAGER_H
