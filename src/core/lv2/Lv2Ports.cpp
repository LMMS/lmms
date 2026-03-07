/*
 * Lv2Ports.cpp - Lv2 port classes implementation
 *
 * Copyright (c) 2019-2020 Johannes Lorenz <jlsf2013$users.sourceforge.net, $=@>
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


#include "Lv2Ports.h"

#ifdef LMMS_HAVE_LV2

#include <lv2/atom/atom.h>
#include <lv2/port-props/port-props.h>

#include "Engine.h"
#include "Lv2Basics.h"
#include "Lv2Manager.h"
#include "Lv2Evbuf.h"
#include "SampleFrame.h"


namespace lmms::Lv2Ports
{




const char *toStr(Flow pf)
{
	switch(pf)
	{
		case Flow::Unknown: return "unknown";
		case Flow::Input: return "input";
		case Flow::Output: return "output";
	}
	return "";
}




const char *toStr(Type pt)
{
	switch(pt)
	{
		case Type::Unknown: return "unknown";
		case Type::Control: return "control";
		case Type::Audio: return "audio";
		case Type::AtomSeq: return "atom-sequence";
		case Type::Cv: return "cv";
	}
	return "";
}




const char *toStr(Vis pv)
{
	switch(pv)
	{
		case Vis::Toggled: return "toggled";
		case Vis::Enumeration: return "enumeration";
		case Vis::Integer: return "integer";
		case Vis::Generic: return "none";
	}
	return "";
}




std::vector<PluginIssue> Meta::get(const LilvPlugin *plugin,
									std::size_t portNum)
{
	std::vector<PluginIssue> portIssues;
	auto issue = [&portIssues](PluginIssueType i, std::string msg = "") {
		portIssues.emplace_back(i, std::move(msg)); };

	Lv2Manager* man = Engine::getLv2Manager();

	const LilvPort* lilvPort = lilv_plugin_get_port_by_index(
								plugin, static_cast<uint32_t>(portNum));

	auto portFunc = [&plugin, &lilvPort, &man](
		bool (*fptr)(const LilvPlugin*, const LilvPort*, const LilvNode*),
		const char* str) {
		return fptr(plugin, lilvPort, man->uri(str).get());
	};

	auto hasProperty = [&portFunc](const char* str) {
		return portFunc(lilv_port_has_property, str); };
	auto isA = [&portFunc](const char* str) {
		return portFunc(lilv_port_is_a, str); };

	const std::string portName = stdStringFromPortName(plugin, lilvPort);

	m_optional = hasProperty(LV2_CORE__connectionOptional);

	m_vis = hasProperty(LV2_CORE__toggled)
		? Vis::Toggled
		: hasProperty(LV2_CORE__enumeration)
		? Vis::Enumeration
		: hasProperty(LV2_CORE__integer)
		? Vis::Integer // WARNING: this may still be changed below
		: Vis::Generic;

	if (isA(LV2_CORE__InputPort)) { m_flow = Flow::Input; }
	else if (isA(LV2_CORE__OutputPort)) { m_flow = Flow::Output; }
	else {
		m_flow = Flow::Unknown;
		issue(PluginIssueType::UnknownPortFlow, portName);
	}

	m_def = .0f;
	m_min = std::numeric_limits<decltype(m_min)>::lowest();
	m_max = std::numeric_limits<decltype(m_max)>::max();
	auto m_min_set = [this]{ return m_min != std::numeric_limits<decltype(m_min)>::lowest(); };
	auto m_max_set = [this]{ return m_max != std::numeric_limits<decltype(m_max)>::max(); };

	m_type = Type::Unknown;
	if (isA(LV2_CORE__ControlPort) || isA(LV2_CORE__CVPort))
	{
		// Read metadata for control ports
		// CV ports are mostly the same as control ports, so we take
		// mostly the same metadata

		if (isA(LV2_CORE__CVPort))
		{
			// currently not supported, but we can still check the metadata
			issue(PluginIssueType::BadPortType, "cvPort");
		}

		m_type = isA(LV2_CORE__CVPort) ? Type::Cv : Type::Control;

		bool isToggle = m_vis == Vis::Toggled;

		LilvNode * defN, * minN = nullptr, * maxN = nullptr;
		lilv_port_get_range(plugin, lilvPort, &defN,
				isToggle ? nullptr : &minN,
				isToggle ? nullptr : &maxN);
		AutoLilvNode def(defN), min(minN), max(maxN);

		auto takeRangeValue = [&](LilvNode* node,
			float& storeHere, PluginIssueType it)
		{
			if (node) { storeHere = lilv_node_as_float(node); }
			else
			{
				// CV ports do not require ranges
				if(m_flow == Flow::Input && m_type != Type::Cv)
				{
					issue(it, portName);
				}
			}
		};

		takeRangeValue(def.get(), m_def, PluginIssueType::PortHasNoDef);
		if (isToggle)
		{
			m_min = .0f;
			m_max = 1.f;
			if(def.get() && m_def != m_min && m_def != m_max)
			{
				issue(PluginIssueType::DefaultValueNotInRange, portName);
			}
		}
		else
		{
			// take min/max
			takeRangeValue(min.get(), m_min, PluginIssueType::PortHasNoMin);
			takeRangeValue(max.get(), m_max, PluginIssueType::PortHasNoMax);
			if(m_type == Type::Cv)
			{
				// no range is allowed and bashed to [-1,+1],
				// but only min or only max does not make sense
				if(!m_min_set() && !m_max_set())
				{
					m_min = -1.f;
					m_max = +1.f;
				}
				else if(!m_min_set()) { issue(PluginIssueType::PortHasNoMin, portName); }
				else if(!m_max_set()) { issue(PluginIssueType::PortHasNoMax, portName); }
			}
			if(m_min > m_max) { issue(PluginIssueType::MinGreaterMax, portName); }

			// sampleRate
			if (hasProperty(LV2_CORE__sampleRate)) { m_sampleRate = true; }

			// default value
			if (def.get())
			{
				if (m_def < m_min) { issue(PluginIssueType::DefaultValueNotInRange, portName); }
				else if (m_def > m_max)
				{
					if(m_sampleRate)
					{
						// multiplying with sample rate will hopefully lead us
						// to a good default value
					}
					else { issue(PluginIssueType::DefaultValueNotInRange, portName); }
				}
			}

			// visualization
			if (!m_min_set() ||
				!m_max_set() ||
				// if we get here, min and max are set, so max-min should not overflow:
				(m_vis == Vis::Integer && m_max - m_min > 15.0f))
			{
				// range too large for spinbox visualisation, use knobs
				// e.g. 0...15 would be OK
				m_vis = Vis::Generic;
			}
		}
	}
	else if (isA(LV2_CORE__AudioPort)) { m_type = Type::Audio; }
	else if (isA(LV2_ATOM__AtomPort))
	{
		AutoLilvNode uriAtomSequence(Engine::getLv2Manager()->uri(LV2_ATOM__Sequence));
		AutoLilvNode uriAtomBufferType(Engine::getLv2Manager()->uri(LV2_ATOM__bufferType));
		AutoLilvNodes bufferTypes(lilv_port_get_value(plugin, lilvPort, uriAtomBufferType.get()));

		if (lilv_nodes_contains(bufferTypes.get(), uriAtomSequence.get()))
		{
			// we accept all kinds of atom sequence ports, even if they take or
			// offer atom types that we do not support:
			// * atom input ports only say what *can* be input, but not what is
			//   required as input
			// * atom output ports only say what *can* be output, but not what must
			//   be evaluated
			m_type = Type::AtomSeq;
		}
	}

	if(m_type == Type::Unknown)
	{
		if (m_optional) { m_used = false; }
		else {
			issue(PluginIssueType::UnknownPortType, portName);
		}
	}

	if (hasProperty(LV2_PORT_PROPS__logarithmic))
	{
		// check min/max available
		// we require them anyways, but this will detect plugins that will
		// be non-Lv2-conforming
		if(m_min == std::numeric_limits<decltype(m_min)>::lowest())
		{
			issue(PluginIssueType::LogScaleMinMissing, portName);
		}
		if(m_max == std::numeric_limits<decltype(m_max)>::max())
		{
			issue(PluginIssueType::LogScaleMaxMissing, portName);
		}
		// forbid min < 0 < max
		if(m_min < 0.f && m_max > 0.f)
		{
			issue(PluginIssueType::LogScaleMinMaxDifferentSigns, portName);
		}
		m_logarithmic = true;
	}

	return portIssues;
}




QString PortBase::name() const
{
	AutoLilvNode node(lilv_port_get_name(m_plugin, m_port));
	QString res = lilv_node_as_string(node.get());
	return res;
}




QString PortBase::uri() const
{
	return lilv_node_as_string(lilv_port_get_symbol(m_plugin, m_port));
}




Audio::Audio(std::size_t bufferSize, bool isSidechain)
	: m_buffer(bufferSize), m_sidechain(isSidechain)
{
}




void Audio::copyBuffersFromCore(const SampleFrame* lmmsBuf,
	unsigned channel, fpp_t frames)
{
	for (std::size_t f = 0; f < static_cast<unsigned>(frames); ++f)
	{
		m_buffer[f] = lmmsBuf[f][channel];
	}
}




void Audio::averageWithBuffersFromCore(const SampleFrame* lmmsBuf,
	unsigned channel, fpp_t frames)
{
	for (std::size_t f = 0; f < static_cast<unsigned>(frames); ++f)
	{
		m_buffer[f] = (m_buffer[f] + lmmsBuf[f][channel]) / 2.0f;
	}
}




void Audio::copyBuffersToCore(SampleFrame* lmmsBuf,
	unsigned channel, fpp_t frames) const
{
	for (std::size_t f = 0; f < static_cast<unsigned>(frames); ++f)
	{
		lmmsBuf[f][channel] = m_buffer[f];
	}
}




void AtomSeq::Lv2EvbufDeleter::operator()(LV2_Evbuf *n) { lv2_evbuf_free(n); }




} // namespace lmms::Lv2Ports

#endif // LMMS_HAVE_LV2

