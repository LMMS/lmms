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

#include "AudioEngine.h"
#include "Model.h"

namespace lmms
{

PluginPinConnector::PluginPinConnector(Model* parent)
	: QObject{parent}
{
}

PluginPinConnector::PluginPinConnector(int pluginInCount, int pluginOutCount, Model* parent)
	: QObject{parent}
	, m_pluginInCount{pluginInCount}
	, m_pluginOutCount{pluginOutCount}
{
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

	setChannelCount(inCount, m_inModels, m_pluginInCount);
	setChannelCount(outCount, m_outModels, m_pluginOutCount);

	updateOptions();

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
	assert(frames < DEFAULT_BUFFER_SIZE);

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

				if (outputEnabled(inChannel, outChannel))
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
	// Only plugins with a mono in/out need to be saved
	//if (m_portCountIn != 1 && m_portCountOut != 1) { return; }

	elem.setAttribute("in_count", m_pluginInCount);   // probably not needed, but just in case
	elem.setAttribute("out_count", m_pluginOutCount); // ditto

	auto pinsIn = doc.createElement("in");
	elem.appendChild(pinsIn);
	saveSettings(m_inModels, doc, pinsIn);

	auto pinsOut = doc.createElement("out");
	elem.appendChild(pinsOut);
	saveSettings(m_outModels, doc, pinsOut);
}

void PluginPinConnector::loadSettings(const QDomElement& elem)
{
	// Until full routing support is added, track channel count should always be 2
	assert(m_coreInCount == 2);
	assert(m_coreOutCount == 2);

	// TODO: Assert port counts are what was expected?
	const auto pluginInCount = elem.attribute("in_count", "0").toInt();
	const auto pluginOutCount = elem.attribute("out_count", "0").toInt();
	setChannelCounts(pluginInCount, pluginOutCount);

	loadSettings(elem.firstChildElement("in"), m_inModels);
	loadSettings(elem.firstChildElement("out"), m_outModels);
}

void PluginPinConnector::saveSettings(const PinMap& pins, QDomDocument& doc, QDomElement& elem)
{
	for (std::size_t trackChannel = 0; trackChannel < pins.size(); ++trackChannel)
	{
		auto& trackChannels = pins[trackChannel];
		for (std::size_t pluginChannel = 0; pluginChannel < trackChannels.size(); ++pluginChannel)
		{
			if (trackChannels[pluginChannel]->value())
			{
				trackChannels[pluginChannel]->saveSettings(doc, elem,
					QString{"%1,%2"}.arg(trackChannel).arg(pluginChannel));
			}
		}
	}
}

void PluginPinConnector::loadSettings(const QDomElement& elem, PinMap& pins)
{
	for (std::size_t trackChannel = 0; trackChannel < pins.size(); ++trackChannel)
	{
		auto& trackChannels = pins[trackChannel];
		for (std::size_t pluginChannel = 0; pluginChannel < trackChannels.size(); ++pluginChannel)
		{
			const auto name = QString{"%1,%2"}.arg(trackChannel).arg(pluginChannel);
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
}

void PluginPinConnector::setChannelCount(int newCount, PluginPinConnector::PinMap& models, int& oldCount)
{
	auto parent = dynamic_cast<Model*>(this->parent());
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
	throw std::runtime_error{"Not implemented yet"};
}

} // namespace lmms
