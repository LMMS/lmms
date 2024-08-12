/*
 * PluginPinConnector.cpp - Specifies how to route audio channels
 *                          in and out of a plugin.
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

#include "PluginPinConnector.h"

#include <QDomDocument>
#include <QDomElement>
#include <cassert>
#include <stdexcept>
#include <iostream>

#include "AudioEngine.h"
#include "Model.h"

namespace lmms
{

PluginPinConnector::PluginPinConnector(Model* parent)
	: QObject{parent}
{
	std::cout << "~~~Default ctor\n";

#ifndef NDEBUG
	std::cout << "~~~DEBUG ENBALED\n";
#endif

	//setChannelCounts(DEFAULT_CHANNELS, DEFAULT_CHANNELS);
	//setDefaultConnections();
}

PluginPinConnector::PluginPinConnector(int pluginInCount, int pluginOutCount, Model* parent)
	: QObject{parent}
{
	std::cout << "~~~2nd ctor\n";

#ifndef NDEBUG
	std::cout << "~~~DEBUG ENABLED\n";
#endif

	setChannelCounts(pluginInCount, pluginOutCount);
	setDefaultConnections();
}

void PluginPinConnector::setChannelCounts(int inCount, int outCount)
{
	if (inCount < 0)
	{
		throw std::invalid_argument{"Invalid input count"};
	}

	if (outCount < 0)
	{
		throw std::invalid_argument{"Invalid output count"};
	}

	if (inCount == 0 && outCount == 0)
	{
		throw std::invalid_argument{"At least one port count must be non-zero"};
	}

	if (m_pluginInCount == inCount && m_pluginOutCount == outCount)
	{
		// No action needed
		return;
	}

	std::cout << "BEFORE\n";
	std::cout << "~~~~~inCount:" << inCount << "; outCount:" << outCount << "\n";
	std::cout << "~~~~~m_inCount:" << m_pluginInCount << "; m_outCount:" << m_pluginOutCount << "\n";
	std::cout << "~~~~~inModels.size():" << m_inModels.size() << "; inModels[0].size():" << (m_inModels.size() > 0 ? (int)m_inModels[0].size() : 0) << "\n";
	std::cout << "~~~~~outModels.size():" << m_outModels.size() << "; outModels[0].size():" << (m_outModels.size() > 0 ? (int)m_outModels[0].size() : 0) << "\n";

	setChannelCount(inCount, m_inModels, m_pluginInCount);
	setChannelCount(outCount, m_outModels, m_pluginOutCount);

	std::cout << "AFTER\n";
	std::cout << "~~~~~m_inCount:" << m_pluginInCount << "; m_outCount:" << m_pluginOutCount << "\n";
	std::cout << "~~~~~inModels.size():" << m_inModels.size() << "; inModels[0].size():" << (m_inModels.size() > 0 ? (int)m_inModels[0].size() : 0) << "\n";
	std::cout << "~~~~~outModels.size():" << m_outModels.size() << "; outModels[0].size():" << (m_outModels.size() > 0 ? (int)m_outModels[0].size() : 0) << "\n";

	updateOptions();

	setDefaultConnections(); // TEMPORARY

	emit channelCountsChanged();
}

void PluginPinConnector::setChannelCountIn(int inCount)
{
	setChannelCounts(inCount, m_pluginOutCount);
}

void PluginPinConnector::setChannelCountOut(int outCount)
{
	setChannelCounts(m_pluginInCount, outCount);
}

void PluginPinConnector::setDefaultConnections()
{
	// Assumes pin maps are cleared
	// TODO: Take into account channel groups?

	assert(m_coreInCount >= 2);
	assert(m_coreOutCount >= 2);

	switch (m_pluginInCount)
	{
		case 0: break;
		case 1:
			m_inModels[0][0]->setValue(true);
			m_inModels[1][0]->setValue(true);
			break;
		default: // >= 2
			m_inModels[0][0]->setValue(true);
			m_inModels[1][1]->setValue(true);
			break;
	}

	switch (m_pluginOutCount)
	{
		case 0: break;
		case 1:
			m_outModels[0][0]->setValue(true);
			m_outModels[1][0]->setValue(true);
			break;
		default: // >= 2
			m_outModels[0][0]->setValue(true);
			m_outModels[1][1]->setValue(true);
			break;
	}
}

void PluginPinConnector::routeToPlugin(f_cnt_t frames, CoreAudioData in, SplitAudioData<sample_t> out)
{
	// Zero the output buffer
	std::fill_n(out.ptr, out.size, 0.f);

	std::uint8_t outChannel = 0;
	for (f_cnt_t outSampleIdx = 0; outSampleIdx < out.size; outSampleIdx += frames, ++outChannel)
	{
		mix_ch_t numRouted = 0; // counter for # of in channels routed to the current out channel
		sample_t* outPtr = &out[outSampleIdx];

		for (std::uint8_t inChannelPairIdx = 0; inChannelPairIdx < in.size; ++inChannelPairIdx)
		{
			const sampleFrame* inPtr = in[inChannelPairIdx]; // L/R track channel pair

			const std::uint8_t inChannel = inChannelPairIdx * 2;
			const std::uint8_t enabledPins =
				(static_cast<std::uint8_t>(inputEnabled(inChannel, outChannel)) << 1u)
				+ static_cast<std::uint8_t>(inputEnabled(inChannel + 1, outChannel));

			switch (enabledPins)
			{
				case 0b00: break;
				case 0b01: // R channel only
				{
					for (f_cnt_t frame = 0; frame < frames; ++frame)
					{
						outPtr[frame] += inPtr[frame][1];
					}
					++numRouted;
					break;
				}
				case 0b10: // L channel only
				{
					for (f_cnt_t frame = 0; frame < frames; ++frame)
					{
						outPtr[frame] += inPtr[frame][0];
					}
					++numRouted;
					break;
				}
				case 0b11: // Both channels
				{
					for (f_cnt_t frame = 0; frame < frames; ++frame)
					{
						outPtr[frame] += inPtr[frame][0] + inPtr[frame][1];
					}
					numRouted += 2;
					break;
				}
				default:
					unreachable();
					break;
			}
		}

		// No input channels were routed to this output - output stays zeroed
		if (numRouted == 0) { continue; }

		// Normalize output
		for (f_cnt_t frame = 0; frame < frames; ++frame)
		{
			outPtr[frame] /= numRouted;
		}
	}
}

void PluginPinConnector::routeFromPlugin(f_cnt_t frames, SplitAudioData<const sample_t> in, CoreAudioDataMut inOut)
{
	assert(frames <= DEFAULT_BUFFER_SIZE);

	for (std::uint8_t outChannelPairIdx = 0; outChannelPairIdx < inOut.size; ++outChannelPairIdx)
	{
		auto buffer = std::array<float, DEFAULT_BUFFER_SIZE>();
		mix_ch_t numRouted; // counter for # of in channels routed to the current out channel

		// Helper function - returns `numRouted` and `buffer` via out parameter
		const auto mixInputs = [&](std::uint8_t outChannel, mix_ch_t& numRoutedOut, auto& bufferOut) {
			bufferOut.fill(0);
			numRoutedOut = 0;

			std::uint8_t inChannel = 0;
			for (f_cnt_t inSampleIdx = 0; inSampleIdx < in.size; inSampleIdx += frames, ++inChannel)
			{
				const sample_t* inPtr = &in[inSampleIdx];

				if (outputEnabled(outChannel, inChannel))
				{
					for (f_cnt_t frame = 0; frame < frames; ++frame)
					{
						bufferOut[frame] += inPtr[frame];
					}
					++numRoutedOut;
				}
			}
		};

		// Left SampleFrame channel first

		auto outChannel = static_cast<std::uint8_t>(outChannelPairIdx * 2);

		mixInputs(outChannel, numRouted, buffer);
		if (numRouted > 0)
		{
			// Normalize output
			sampleFrame* outPtr = inOut[outChannel];
			for (f_cnt_t frame = 0; frame < frames; ++frame)
			{
				outPtr[frame][0] = buffer[frame] / numRouted;
			}
		}

		// Right SampleFrame channel second

		++outChannel;

		mixInputs(outChannel, numRouted, buffer);
		if (numRouted > 0)
		{
			// Normalize output
			sampleFrame* outPtr = inOut[outChannel];
			for (f_cnt_t frame = 0; frame < frames; ++frame)
			{
				outPtr[frame][1] = buffer[frame] / numRouted;
			}
		}

		// NOTE: When numRouted == 0, nothing needs to be written to `inOut` for
		// the audio bypass, since it already contains the LMMS core input audio.
	}
}

void PluginPinConnector::saveSettings(QDomDocument& doc, QDomElement& elem)
{
	elem.setAttribute("v", 0); // version

	elem.setAttribute("ins", m_pluginInCount);
	elem.setAttribute("outs", m_pluginOutCount);

	auto pinsIn = doc.createElement("in_matrix");
	elem.appendChild(pinsIn);
	saveSettings(m_inModels, doc, pinsIn);

	auto pinsOut = doc.createElement("out_matrix");
	elem.appendChild(pinsOut);
	saveSettings(m_outModels, doc, pinsOut);
}

void PluginPinConnector::loadSettings(const QDomElement& elem)
{
	// Until full routing support is added, track channel count should always be 2
	assert(m_coreInCount == 2);
	assert(m_coreOutCount == 2);

	// TODO: Assert port counts are what was expected?
	const auto pluginInCount = elem.attribute("ins", "0").toInt();
	const auto pluginOutCount = elem.attribute("outs", "0").toInt();
	setChannelCounts(pluginInCount, pluginOutCount);

	loadSettings(elem.firstChildElement("in_matrix"), m_inModels);
	loadSettings(elem.firstChildElement("out_matrix"), m_outModels);
}

void PluginPinConnector::saveSettings(const PinMap& pins, QDomDocument& doc, QDomElement& elem)
{
	// Only saves connections that are actually used, otherwise could bloat project file
	for (std::size_t trackChannel = 0; trackChannel < pins.size(); ++trackChannel)
	{
		auto& trackChannels = pins[trackChannel];
		for (std::size_t pluginChannel = 0; pluginChannel < trackChannels.size(); ++pluginChannel)
		{
			if (trackChannels[pluginChannel]->value() || trackChannels[pluginChannel]->isAutomatedOrControlled())
			{
				trackChannels[pluginChannel]->saveSettings(doc, elem,
					QString{"c%1,%2"}.arg(trackChannel + 1).arg(pluginChannel + 1));
			}
		}
	}
}

void PluginPinConnector::loadSettings(const QDomElement& elem, PinMap& pins)
{
	const auto trackChannelCount = static_cast<int>(pins.size());
	const auto pluginChannelCount = trackChannelCount > 0 ? static_cast<int>(pins[0].size()) : 0;

	std::vector<std::vector<bool>> connectionsToLoad;
	connectionsToLoad.resize(trackChannelCount);
	for (auto& pluginChannels : connectionsToLoad)
	{
		pluginChannels.resize(pluginChannelCount);
	}

	auto addConnection = [&connectionsToLoad](const QString& name) {
#ifndef NDEBUG
		constexpr auto minSize = std::string_view{"c#,#"}.size();
		if (name.size() < minSize) { throw std::runtime_error{"string too small"}; }
		if (name[0] != 'c') { throw std::runtime_error{"invalid string"}; }
#endif

		const auto pos = name.indexOf(QStringView{u","});
#ifndef NDEBUG
		if (pos <= 0) { throw std::runtime_error{"parse failure"}; }
#endif

		auto trackChannel = name.midRef(1, pos - 1).toInt();
		auto pluginChannel = name.midRef(pos + 1).toInt();
#ifndef NDEBUG
		if (trackChannel == 0 || pluginChannel == 0) { throw std::runtime_error{"failed to parse integer"}; }
#endif

		connectionsToLoad.at(trackChannel - 1).at(pluginChannel - 1) = true;
	};

	// Get non-automated pin connector connections
	const auto& attrs = elem.attributes();
	for (int idx = 0; idx < attrs.size(); ++idx)
	{
		const auto node = attrs.item(idx);
		addConnection(node.nodeName());
	}

	// Get automated pin connector connections
	const auto children = elem.childNodes();
	for (int idx = 0; idx < children.size(); ++idx)
	{
		const auto node = children.item(idx);
		addConnection(node.nodeName());
	}

	// Lastly, load the connections
	for (std::size_t trackChannel = 0; trackChannel < pins.size(); ++trackChannel)
	{
		auto& trackChannels = pins[trackChannel];
		auto& trackChannelsToLoad = connectionsToLoad[trackChannel];
		for (std::size_t pluginChannel = 0; pluginChannel < trackChannels.size(); ++pluginChannel)
		{
			if (trackChannelsToLoad[pluginChannel])
			{
				const auto name = QString{"c%1,%2"}.arg(trackChannel + 1).arg(pluginChannel + 1);
				trackChannels[pluginChannel]->loadSettings(elem, name);
			}
			else
			{
				trackChannels[pluginChannel]->setValue(0);
			}
		}
	}

#if 0
	// TODO: This method is very inefficient
	for (std::size_t trackChannel = 0; trackChannel < pins.size(); ++trackChannel)
	{
		auto& trackChannels = pins[trackChannel];
		for (std::size_t pluginChannel = 0; pluginChannel < trackChannels.size(); ++pluginChannel)
		{
			const auto name = QString{"c%1,%2"}.arg(trackChannel).arg(pluginChannel);
			if (elem.hasAttribute(name) || !elem.firstChildElement(name).isNull())
			{
				trackChannels[pluginChannel]->loadSettings(elem, name);
			}
			else
			{
				trackChannels[pluginChannel]->setValue(0);
			}
		}
	}
#endif
}

void PluginPinConnector::setChannelCount(int newCount, PluginPinConnector::PinMap& models, int& oldCount)
{
	// TODO: Move this to a separate method?
	if (models.size() != m_coreOutCount)
	{
		models.resize(m_coreOutCount);
	}

	auto parent = dynamic_cast<Model*>(this->parent());
	assert(parent != nullptr);
	if (oldCount < newCount)
	{
		for (auto& pluginModels : models)
		{
			pluginModels.reserve(newCount);
			for (int idx = oldCount; idx < newCount; ++idx)
			{
				pluginModels.push_back(new BoolModel{false, parent});
			}
		}
	}
	else if (oldCount > newCount)
	{
		for (auto& pluginModels : models)
		{
			for (int idx = newCount; idx < oldCount; ++idx)
			{
				delete pluginModels[idx];
			}
			pluginModels.erase(pluginModels.begin() + newCount, pluginModels.end());
		}
	}
	oldCount = newCount;
};

auto PluginPinConnector::instantiateView(QWidget* parent) -> gui::PluginPinConnectorWidget*
{
	throw std::runtime_error{"Not implemented yet"};
}

void PluginPinConnector::updateOptions()
{
	//throw std::runtime_error{"Not implemented yet"};
}

} // namespace lmms
