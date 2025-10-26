/*
 * Lv2Manager.cpp - Implementation of Lv2Manager class
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

#include "Lv2Manager.h"

#ifdef LMMS_HAVE_LV2

#include <algorithm>
#include <cstdlib>
#include <lilv/lilv.h>
#include <lv2/buf-size/buf-size.h>
#include <lv2/options/options.h>
#include <lv2/worker/worker.h>
#include <QDebug>
#include <QElapsedTimer>

#include "AudioEngine.h"
#include "ConfigManager.h"
#include "Engine.h"
#include "Plugin.h"
#include "Lv2ControlBase.h"
#include "Lv2Options.h"
#include "PluginIssue.h"


namespace lmms
{


const std::set<std::string_view> Lv2Manager::unstablePlugins =
{
	// github.com/calf-studio-gear/calf, #278
	"http://calf.sourceforge.net/plugins/Analyzer",
	"http://calf.sourceforge.net/plugins/BassEnhancer",
	"http://calf.sourceforge.net/plugins/CompensationDelay",
	"http://calf.sourceforge.net/plugins/Crusher",
	"http://calf.sourceforge.net/plugins/Exciter",
	"http://calf.sourceforge.net/plugins/Saturator",
	"http://calf.sourceforge.net/plugins/StereoTools",
	"http://calf.sourceforge.net/plugins/TapeSimulator",
	"http://calf.sourceforge.net/plugins/TransientDesigner",
	"http://calf.sourceforge.net/plugins/Vinyl",

	// https://gitlab.com/drobilla/blop-lv2/-/issues/3
	"http://drobilla.net/plugins/blop/pulse",
	"http://drobilla.net/plugins/blop/sawtooth",
	"http://drobilla.net/plugins/blop/square",
	"http://drobilla.net/plugins/blop/triangle",

	// unstable
	"urn:juced:DrumSynth"
};

const std::set<std::string_view> Lv2Manager::pluginsOnlyUsefulWithUi =
{
	// Visualization, meters, and scopes etc., won't work if UI is disabled
	"http://distrho.sf.net/plugins/ProM",
	"http://distrho.sf.net/plugins/glBars",
	"http://gareus.org/oss/lv2/meters#spectr30mono",
	"http://gareus.org/oss/lv2/meters#spectr30stereo",
	"http://gareus.org/oss/lv2/meters#bitmeter",
	"http://gareus.org/oss/lv2/meters#BBCM6",
	"http://gareus.org/oss/lv2/meters#BBCmono",
	"http://gareus.org/oss/lv2/meters#BBCstereo",
	"http://gareus.org/oss/lv2/meters#DINmono",
	"http://gareus.org/oss/lv2/meters#DINstereo",
	"http://gareus.org/oss/lv2/meters#EBUmono",
	"http://gareus.org/oss/lv2/meters#EBUstereo",
	"http://gareus.org/oss/lv2/meters#EBUr128",
	"http://gareus.org/oss/lv2/meters#BBCM6",
	"http://gareus.org/oss/lv2/meters#dr14mono",
	"http://gareus.org/oss/lv2/meters#dr14stereo",
	"http://gareus.org/oss/lv2/meters#K12mono",
	"http://gareus.org/oss/lv2/meters#K12stereo",
	"http://gareus.org/oss/lv2/meters#K14mono",
	"http://gareus.org/oss/lv2/meters#K14stereo",
	"http://gareus.org/oss/lv2/meters#K20mono",
	"http://gareus.org/oss/lv2/meters#K20stereo",
	"http://gareus.org/oss/lv2/meters#NORmono",
	"http://gareus.org/oss/lv2/meters#NORstereo",
	"http://gareus.org/oss/lv2/meters#COR",
	"http://gareus.org/oss/lv2/meters#dBTPmono",
	"http://gareus.org/oss/lv2/meters#dBTPstereo",
	"http://gareus.org/oss/lv2/meters#TPnRMSmono",
	"http://gareus.org/oss/lv2/meters#TPnRMSstereo",
	"http://gareus.org/oss/lv2/meters#VUmono",
	"http://gareus.org/oss/lv2/meters#VUstereo",
	"http://gareus.org/oss/lv2/meters#goniometer",
	"http://gareus.org/oss/lv2/meters#stereoscope",
	"http://gareus.org/oss/lv2/meters#SigDistHist",
	"http://gareus.org/oss/lv2/tuna#one",
	"http://gareus.org/oss/lv2/tuna#two",
	"http://gareus.org/oss/lv2/sisco#Mono",
	"http://gareus.org/oss/lv2/sisco#Stereo",
	"http://gareus.org/oss/lv2/spectra#Mono",
	"http://gareus.org/oss/lv2/convoLV2#Mono",
	"http://gareus.org/oss/lv2/convoLV2#MonoToStereo",
	"http://gareus.org/oss/lv2/convoLV2#Stereo",
	"http://gareus.org/oss/lv2/zeroconvolv#CfgMono",
	"http://gareus.org/oss/lv2/zeroconvolv#CfgMonoToStereo",
	"http://gareus.org/oss/lv2/zeroconvolv#CfgStereo",
	"http://gareus.org/oss/lv2/zeroconvolv#Mono",
	"http://gareus.org/oss/lv2/zeroconvolv#MonoToStereo",
	"http://gareus.org/oss/lv2/zeroconvolv#Stereo",
	"http://lsp-plug.in/plugins/lv2/latency_meter",
	"http://lsp-plug.in/plugins/lv2/spectrum_analyzer_x1",
	"http://lsp-plug.in/plugins/lv2/spectrum_analyzer_x2",
	"http://lsp-plug.in/plugins/lv2/phase_detector",
	"http://lsp-plug.in/plugins/lv2/profiler_mono",
	"http://lsp-plug.in/plugins/lv2/profiler_stereo",
	"http://invadarecords.com/plugins/lv2/meter",
	"http://guitarix.sourceforge.net/plugins/gxtuner#tuner",
	"https://github.com/jpcima/ADLplug",
	"https://github.com/HiFi-LoFi/KlangFalter",
	"https://github.com/klangfreund/SpectrumAnalyser",
	"https://github.com/klangfreund/lufsmeter",
	"https://github.com/laixinyuan/StereoSourceSepartion",
	"urn:juce:TalFilter2",
	"urn:juce:Vex",
	"http://zynaddsubfx.sourceforge.net",
	"http://geontime.com/geonkick/single"
};

const std::set<std::string_view> Lv2Manager::unstablePluginsBuffersizeLessEqual32 =
{
	"http://moddevices.com/plugins/mod-devel/2Voices",
	"http://moddevices.com/plugins/mod-devel/Capo",
	"http://moddevices.com/plugins/mod-devel/Drop",
	"http://moddevices.com/plugins/mod-devel/Harmonizer",
	"http://moddevices.com/plugins/mod-devel/Harmonizer2",
	"http://moddevices.com/plugins/mod-devel/HarmonizerCS",
	"http://moddevices.com/plugins/mod-devel/SuperCapo",
	"http://moddevices.com/plugins/mod-devel/SuperWhammy",
	"http://moddevices.com/plugins/mod-devel/Gx2Voices",
	"http://moddevices.com/plugins/mod-devel/GxCapo",
	"http://moddevices.com/plugins/mod-devel/GxDrop",
	"http://moddevices.com/plugins/mod-devel/GxHarmonizer",
	"http://moddevices.com/plugins/mod-devel/GxHarmonizer2",
	"http://moddevices.com/plugins/mod-devel/GxHarmonizerCS",
	"http://moddevices.com/plugins/mod-devel/GxSuperCapo",
	"http://moddevices.com/plugins/mod-devel/GxSuperWhammy"
};




Lv2Manager::Lv2Manager() :
	m_uridCache(m_uridMap)
{
	const char* dbgStr = getenv("LMMS_LV2_DEBUG");
	m_debug = (dbgStr && *dbgStr);

	m_world = lilv_world_new();
	lilv_world_load_all(m_world);

	m_supportedFeatureURIs.insert(LV2_URID__map);
	m_supportedFeatureURIs.insert(LV2_URID__unmap);
	m_supportedFeatureURIs.insert(LV2_OPTIONS__options);
	m_supportedFeatureURIs.insert(LV2_WORKER__schedule);
	// min/max is always passed in the options
	m_supportedFeatureURIs.insert(LV2_BUF_SIZE__boundedBlockLength);
	// block length is only changed initially in AudioEngine CTOR
	m_supportedFeatureURIs.insert(LV2_BUF_SIZE__fixedBlockLength);
	if (const auto fpp = Engine::audioEngine()->framesPerPeriod(); (fpp & (fpp - 1)) == 0)  // <=> ffp is power of 2 (for ffp > 0)
	{
		m_supportedFeatureURIs.insert(LV2_BUF_SIZE__powerOf2BlockLength);
	}

	auto supportOpt = [this](Lv2UridCache::Id id)
	{
		Lv2Options::supportOption(uridCache()[id]);
	};
	supportOpt(Lv2UridCache::Id::param_sampleRate);
	supportOpt(Lv2UridCache::Id::bufsz_maxBlockLength);
	supportOpt(Lv2UridCache::Id::bufsz_minBlockLength);
	supportOpt(Lv2UridCache::Id::bufsz_nominalBlockLength);
	supportOpt(Lv2UridCache::Id::bufsz_sequenceSize);
}




Lv2Manager::~Lv2Manager()
{
	lilv_world_free(m_world);
}




AutoLilvNode Lv2Manager::uri(const char *uriStr)
{
	return AutoLilvNode(lilv_new_uri(m_world, uriStr));
}




const LilvPlugin *Lv2Manager::getPlugin(const std::string &uri)
{
	auto itr = m_lv2InfoMap.find(uri);
	return itr == m_lv2InfoMap.end() ? nullptr : itr->second.plugin();
}




const LilvPlugin *Lv2Manager::getPlugin(const QString &uri)
{
	return getPlugin(uri.toStdString());
}




void Lv2Manager::initPlugins()
{
	const LilvPlugins* plugins = lilv_world_get_all_plugins(m_world);
	std::size_t pluginCount = 0, pluginsLoaded = 0;
	QElapsedTimer timer;
	timer.start();

	unsigned blocked = 0;
	LILV_FOREACH(plugins, itr, plugins)
	{
		const LilvPlugin* curPlug = lilv_plugins_get(plugins, itr);

		std::vector<PluginIssue> issues;
		Plugin::Type type = Lv2ControlBase::check(curPlug, issues);
		std::sort(issues.begin(), issues.end());
		auto last = std::unique(issues.begin(), issues.end());
		issues.erase(last, issues.end());
		if (m_debug && issues.size())
		{
			qDebug() << "Lv2 plugin"
				<< qStringFromPluginNode(curPlug, lilv_plugin_get_name)
				<< "(URI:"
				<< lilv_node_as_uri(lilv_plugin_get_uri(curPlug))
				<< ") can not be loaded:";
			for (const PluginIssue& iss : issues) { qDebug() << "  - " << iss; }
		}

		Lv2Info info(curPlug, type, issues.empty());

		m_lv2InfoMap[lilv_node_as_uri(lilv_plugin_get_uri(curPlug))]
			= std::move(info);
		if(issues.empty()) { ++pluginsLoaded; }
		else
		{
			if(std::any_of(issues.begin(), issues.end(),
				[](const PluginIssue& iss) {
				return iss.type() == PluginIssueType::Blocked; }))
			{
				++blocked;
			}
		}
		++pluginCount;
	}

	qDebug() << "Lv2 plugin SUMMARY:"
		<< pluginsLoaded << "of" << pluginCount << " loaded in"
		<< timer.elapsed() << "msecs.";
	if(pluginsLoaded != pluginCount)
	{
		if (m_debug)
		{
			qDebug() <<
				"If you don't want to see all this debug output, please set\n"
				"  environment variable \"LMMS_LV2_DEBUG\" to empty or\n"
				"  do not set it.";
		}
		else
		{
			qDebug() <<
				"For details about not loaded plugins, please set\n"
				"  environment variable \"LMMS_LV2_DEBUG\" to nonempty.";
		}
	}

	// TODO: might be better in the LMMS core
	if(ConfigManager::enableBlockedPlugins())
	{
		qWarning() <<
			"WARNING! Blocked plugins enabled! If you want to disable them,\n"
			"  please set environment variable \"LMMS_ENABLE_BLOCKED_PLUGINS\" to empty or\n"
			"  do not set it.";
	}
	else if(blocked > 0)
	{
		qDebug() <<
			"Blocked Lv2 Plugins:" << blocked << "of" << pluginCount << "\n"
			"  If you want to enable them (dangerous!), please set\n"
			"  environment variable \"LMMS_ENABLE_BLOCKED_PLUGINS\" to nonempty.";
	}
}




bool Lv2Manager::isFeatureSupported(const char *featName) const
{
	return m_supportedFeatureURIs.find(featName) != m_supportedFeatureURIs.end();
}




AutoLilvNodes Lv2Manager::findNodes(const LilvNode *subject,
	const LilvNode *predicate, const LilvNode *object)
{
	return AutoLilvNodes(lilv_world_find_nodes (m_world, subject, predicate, object));
}




bool Lv2Manager::wantUi()
{
	return false;
}




// unused + untested yet
bool Lv2Manager::isSubclassOf(const LilvPluginClass* clvss, const char* uriStr)
{
	const LilvPluginClasses* allClasses = lilv_world_get_plugin_classes(m_world);
	const LilvPluginClass* root = lilv_world_get_plugin_class(m_world);
	const LilvPluginClass* search = lilv_plugin_classes_get_by_uri(allClasses,
					uri(uriStr).get());

	auto clssEq = [](const LilvPluginClass* pc1,
		const LilvPluginClass* pc2) -> bool
	{
		return lilv_node_equals(
			lilv_plugin_class_get_uri(pc1),
			lilv_plugin_class_get_uri(pc2));
	};
	bool isFound = false;
	while (!(isFound = clssEq(clvss, search)) && !clssEq(clvss, root))
	{
		clvss = lilv_plugin_classes_get_by_uri(allClasses,
			lilv_plugin_class_get_parent_uri(clvss));
	}
	return isFound;
}


} // namespace lmms

#endif // LMMS_HAVE_LV2
