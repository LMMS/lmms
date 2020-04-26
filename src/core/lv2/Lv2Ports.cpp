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

#include "Engine.h"
#include "Lv2Basics.h"
#include "Lv2Manager.h"

namespace Lv2Ports {




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
		case Type::Event: return "event";
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
		case Vis::None: return "none";
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

	m_vis = hasProperty(LV2_CORE__integer)
		? Vis::Integer // WARNING: this may still be changed below
		: hasProperty(LV2_CORE__enumeration)
		? Vis::Enumeration
		: hasProperty(LV2_CORE__toggled)
		? Vis::Toggled
		: Vis::None;

	if (isA(LV2_CORE__InputPort)) { m_flow = Flow::Input; }
	else if (isA(LV2_CORE__OutputPort)) { m_flow = Flow::Output; }
	else {
		m_flow = Flow::Unknown;
		issue(unknownPortFlow, portName);
	}

	m_def = .0f; m_min = .0f; m_max = .0f;

	if (isA(LV2_CORE__ControlPort))
	{
		m_type = Type::Control;

		if (m_flow == Flow::Input)
		{
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
				else { issue(it, portName); }
			};

			takeRangeValue(def.get(), m_def, portHasNoDef);
			if (!isToggle)
			{
				takeRangeValue(min.get(), m_min, portHasNoMin);
				takeRangeValue(max.get(), m_max, portHasNoMax);

				if (m_max - m_min > 15.0f)
				{
					// range too large for spinbox visualisation, use knobs
					// e.g. 0...15 would be OK
					m_vis = Vis::None;
				}
			}
		}
	}
	else if (isA(LV2_CORE__AudioPort)) { m_type = Type::Audio; }
	else if (isA(LV2_CORE__CVPort)) {
		issue(badPortType, "cvPort");
		m_type = Type::Cv;
	} else {
		if (m_optional) { m_used = false; }
		else {
			issue(PluginIssueType::unknownPortType, portName);
			m_type = Type::Unknown;
		}
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




Audio::Audio(std::size_t bufferSize, bool isSidechain, bool isOptional)
	: m_buffer(bufferSize), m_sidechain(isSidechain), m_optional(isOptional)
{
}




void Audio::copyBuffersFromCore(const sampleFrame *lmmsBuf,
	unsigned channel, fpp_t frames)
{
	for (std::size_t f = 0; f < static_cast<unsigned>(frames); ++f)
	{
		m_buffer[f] = lmmsBuf[f][channel];
	}
}




void Audio::addBuffersFromCore(const sampleFrame *lmmsBuf,
	unsigned channel, fpp_t frames)
{
	for (std::size_t f = 0; f < static_cast<unsigned>(frames); ++f)
	{
		m_buffer[f] = (m_buffer[f] + lmmsBuf[f][channel]) / 2.0f;
	}
}




void Audio::copyBuffersToCore(sampleFrame *lmmsBuf,
	unsigned channel, fpp_t frames) const
{
	for (std::size_t f = 0; f < static_cast<unsigned>(frames); ++f)
	{
		lmmsBuf[f][channel] = m_buffer[f];
	}
}




// make the compiler happy, give each class with virtuals
// a function (the destructor here) which is in a cpp file
PortBase::~PortBase() {}
ConstVisitor::~ConstVisitor() {}
Visitor::~Visitor() {}




} // namespace Lv2Ports

#endif // LMMS_HAVE_LV2

