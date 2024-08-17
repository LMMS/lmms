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
#include "PluginPinConnectorView.h"

namespace lmms
{

PluginPinConnector::PluginPinConnector(Model* parent)
	: Model{parent}
{
	updateTrackChannels(s_totalTrackChannels);
}

PluginPinConnector::PluginPinConnector(int pluginInCount, int pluginOutCount, Model* parent)
	: Model{parent}
{
	updateTrackChannels(s_totalTrackChannels);
	setChannelCounts(pluginInCount, pluginOutCount);
}

auto PluginPinConnector::Matrix::channelName(int channel) const -> QString
{
	// Custom name (if supported)
	assert(channel >= 0);
	if (channel < static_cast<int>(channelNames.size()))
	{
		return channelNames[channel];
	}

	// A-Z
	if (channel < 26)
	{
		return QChar::fromLatin1('A' + channel);
	}

	// AA-ZZ
	channel -= 26;
	if (channel < 26 * 26)
	{
		auto ret = QString{"AA"};
		ret[0] = QChar::fromLatin1('A' + channel / 26);
		ret[1] = QChar::fromLatin1('A' + channel % 26);
		return ret;
	}

	throw std::invalid_argument{"Too many channels"};
}

void PluginPinConnector::setChannelCounts(int inCount, int outCount)
{
	if (m_trackChannelsUsed > MaxTrackChannels)
	{
		throw std::runtime_error{"Only up to 256 track channels are allowed"};
	}

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

	if (in().channelCount == inCount && out().channelCount == outCount)
	{
		// No action needed
		return;
	}

	const bool initialSetup = in().channelCount == 0 && out().channelCount == 0;

	setChannelCount(inCount, m_in);
	setChannelCount(outCount, m_out);

	if (initialSetup)
	{
		setDefaultConnections();
	}

	emit propertiesChanged();
}

void PluginPinConnector::setChannelCountIn(int inCount)
{
	setChannelCounts(inCount, out().channelCount);
}

void PluginPinConnector::setChannelCountOut(int outCount)
{
	setChannelCounts(in().channelCount, outCount);
}

void PluginPinConnector::setDefaultConnections()
{
	// Assumes pin maps are cleared
	// TODO: Take into account channel groups?

	assert(m_trackChannelsUsed >= 2);

	auto setConnections = [](Matrix& matrix) {
		switch (matrix.channelCount)
		{
			case 0: break;
			case 1:
				matrix.pins[0][0]->setValue(true);
				matrix.pins[1][0]->setValue(true);
				break;
			default: // >= 2
				matrix.pins[0][0]->setValue(true);
				matrix.pins[1][1]->setValue(true);
				break;
		}
	};

	setConnections(m_in);
	setConnections(m_out);
}

void PluginPinConnector::routeToPlugin(f_cnt_t frames, CoreAudioData in, SplitAudioData<sample_t> out)
{
	// Ignore all unused track channels for better performance
	const auto inSizeConstrained = m_trackChannelsUsed / 2;
	assert(inSizeConstrained <= in.size);

	// Zero the output buffer
	std::fill_n(out.ptr, out.size, 0.f);

	std::uint8_t outChannel = 0;
	for (f_cnt_t outSampleIdx = 0; outSampleIdx < out.size; outSampleIdx += frames, ++outChannel)
	{
		mix_ch_t numRouted = 0; // counter for # of in channels routed to the current out channel
		sample_t* outPtr = &out[outSampleIdx];

		for (std::uint8_t inChannelPairIdx = 0; inChannelPairIdx < inSizeConstrained; ++inChannelPairIdx)
		{
			const SampleFrame* inPtr = in[inChannelPairIdx]; // L/R track channel pair

			const std::uint8_t inChannel = inChannelPairIdx * 2;
			const std::uint8_t enabledPins =
				(static_cast<std::uint8_t>(m_in.enabled(inChannel, outChannel)) << 1u)
				+ static_cast<std::uint8_t>(m_in.enabled(inChannel + 1, outChannel));

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

	// Ignore all unused track channels for better performance
	const auto inOutSizeConstrained = m_trackChannelsUsed / 2;
	assert(inOutSizeConstrained <= inOut.size);

	for (std::uint8_t outChannelPairIdx = 0; outChannelPairIdx < inOutSizeConstrained; ++outChannelPairIdx)
	{
		SampleFrame* outPtr = inOut[outChannelPairIdx]; // L/R track channel pair
		const auto outChannel = static_cast<std::uint8_t>(outChannelPairIdx * 2);

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

				if (m_out.enabled(outChannel, inChannel))
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
		mixInputs(outChannel, numRouted, buffer);
		if (numRouted > 0)
		{
			// Normalize output
			for (f_cnt_t frame = 0; frame < frames; ++frame)
			{
				outPtr[frame][0] = buffer[frame] / numRouted;
			}
		}

		// Right SampleFrame channel second
		mixInputs(outChannel + 1, numRouted, buffer);
		if (numRouted > 0)
		{
			// Normalize output
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
	auto pins = doc.createElement(nodeName());
	elem.appendChild(pins);

	if (m_trackChannelsUsed != s_totalTrackChannels)
	{
		pins.setAttribute("tc_used", m_trackChannelsUsed);
	}

	pins.setAttribute("num_in", in().channelCount);
	pins.setAttribute("num_out", out().channelCount);

	auto pinsIn = doc.createElement("in_matrix");
	pins.appendChild(pinsIn);
	saveSettings(m_in, doc, pinsIn);

	auto pinsOut = doc.createElement("out_matrix");
	pins.appendChild(pinsOut);
	saveSettings(m_out, doc, pinsOut);
}

void PluginPinConnector::loadSettings(const QDomElement& elem)
{
	const auto pins = elem.firstChildElement(nodeName());
	if (pins.isNull()) { return; }

	// Until full routing support is added, track channel count should always be 2
	m_trackChannelsUsed = pins.attribute("tc_used", QString::number(s_totalTrackChannels)).toUInt();
	assert(m_trackChannelsUsed == 2);

	// TODO: Assert port counts are what was expected?
	const auto pluginInCount = pins.attribute("num_in", "0").toInt();
	const auto pluginOutCount = pins.attribute("num_out", "0").toInt();
	setChannelCounts(pluginInCount, pluginOutCount);

	loadSettings(pins.firstChildElement("in_matrix"), m_in);
	loadSettings(pins.firstChildElement("out_matrix"), m_out);
}

void PluginPinConnector::saveSettings(const Matrix& matrix, QDomDocument& doc, QDomElement& elem)
{
	// Only saves connections that are actually used, otherwise could bloat project file
	for (std::size_t trackChannel = 0; trackChannel < matrix.pins.size(); ++trackChannel)
	{
		auto& trackChannels = matrix.pins[trackChannel];
		for (std::size_t pluginChannel = 0; pluginChannel < trackChannels.size(); ++pluginChannel)
		{
			if (trackChannels[pluginChannel]->value() || trackChannels[pluginChannel]->isAutomatedOrControlled())
			{
				trackChannels[pluginChannel]->saveSettings(doc, elem,
					QString{"c%1_%2"}.arg(trackChannel + 1).arg(pluginChannel + 1));
			}
		}
	}
}

void PluginPinConnector::loadSettings(const QDomElement& elem, Matrix& matrix)
{
	const auto trackChannelCount = static_cast<int>(matrix.pins.size());
	const auto pluginChannelCount = trackChannelCount > 0 ? static_cast<int>(matrix.pins[0].size()) : 0;
	assert(pluginChannelCount == matrix.channelCount);

	std::vector<std::vector<bool>> connectionsToLoad;
	connectionsToLoad.resize(trackChannelCount);
	for (auto& pluginChannels : connectionsToLoad)
	{
		pluginChannels.resize(pluginChannelCount);
	}

	auto addConnection = [&connectionsToLoad](const QString& name) {
#ifndef NDEBUG
		constexpr auto minSize = static_cast<int>(std::string_view{"c#_#"}.size());
		if (name.size() < minSize) { throw std::runtime_error{"string too small"}; }
		if (name[0] != 'c') { throw std::runtime_error{"invalid string: \"" + name.toStdString() + "\""}; }
#endif

		const auto pos = name.indexOf('_');
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
	for (std::size_t trackChannel = 0; trackChannel < matrix.pins.size(); ++trackChannel)
	{
		auto& trackChannels = matrix.pins[trackChannel];
		auto& trackChannelsToLoad = connectionsToLoad[trackChannel];
		for (std::size_t pluginChannel = 0; pluginChannel < trackChannels.size(); ++pluginChannel)
		{
			if (trackChannelsToLoad[pluginChannel])
			{
				const auto name = QString{"c%1_%2"}.arg(trackChannel + 1).arg(pluginChannel + 1);
				trackChannels[pluginChannel]->loadSettings(elem, name);
			}
			else
			{
				trackChannels[pluginChannel]->setValue(0);
			}
		}
	}
}

void PluginPinConnector::setChannelCount(int newCount, Matrix& matrix)
{
	auto parent = dynamic_cast<Model*>(this->parent());
	assert(parent != nullptr);

	if (matrix.channelCount < newCount)
	{
		const auto nameFormat = QString::fromUtf16(matrix.in
			? u"Pin in [%1 \U0001F82E %2]"
			: u"Pin out [%2 \U0001F82E %1]");

		for (unsigned tcIdx = 0; tcIdx < matrix.pins.size(); ++tcIdx)
		{
			auto& pluginChannels = matrix.pins[tcIdx];
			pluginChannels.reserve(newCount);
			for (int pcIdx = matrix.channelCount; pcIdx < newCount; ++pcIdx)
			{
				const auto name = nameFormat.arg(tcIdx + 1).arg(matrix.channelName(pcIdx));
				BoolModel* model = pluginChannels.emplace_back(new BoolModel{false, parent, name});
				connect(model, &BoolModel::dataChanged, this, &PluginPinConnector::dataChanged);
			}
		}
	}
	else if (matrix.channelCount > newCount)
	{
		for (auto& pluginChannels : matrix.pins)
		{
			for (int idx = newCount; idx < matrix.channelCount; ++idx)
			{
				delete pluginChannels[idx];
			}
			pluginChannels.erase(pluginChannels.begin() + newCount, pluginChannels.end());
		}
	}
	matrix.channelCount = newCount;
};

auto PluginPinConnector::instantiateView(QWidget* parent) -> gui::PluginPinConnectorView*
{
	return new gui::PluginPinConnectorView{this, parent};
}

auto PluginPinConnector::getChannelCountText() const -> QString
{
	const auto inText = in().channelCount > static_cast<int>(s_totalTrackChannels)
		? QString{"%1/%2"}.arg(s_totalTrackChannels).arg(in().channelCount)
		: QString{"%1"}.arg(in().channelCount);

	const auto outText = out().channelCount > static_cast<int>(s_totalTrackChannels)
		? QString{"%1/%2"}.arg(s_totalTrackChannels).arg(out().channelCount)
		: QString{"%1"}.arg(out().channelCount);

	return QString{tr("%1 in %2 out")}.arg(inText).arg(outText);
}

void PluginPinConnector::updateTrackChannels(int count)
{
	if (count < 2) { throw std::invalid_argument{"There must be at least 2 track channels"}; }
	if (count % 2 != 0) { throw std::invalid_argument{"There must be an even number of track channels"}; }

	if (static_cast<int>(in().pins.size()) == count
		&& static_cast<int>(out().pins.size()) == count)
	{
		return;
	}

	auto updateModels = [&](int pluginChannels, const QString& nameFormat, PluginPinConnector::Matrix& matrix) {
		auto& models = matrix.pins;
		auto oldSize = static_cast<int>(models.size());
		if (oldSize > count)
		{
			for (auto tcIdx = count; tcIdx < oldSize; ++tcIdx)
			{
				for (BoolModel* model : models[tcIdx])
				{
					delete model;
				}
			}
			m_trackChannelsUsed = std::min<unsigned>(m_trackChannelsUsed, count);
			models.resize(count);
		}
		else if (oldSize < count)
		{
			auto parent = dynamic_cast<Model*>(this->parent());
			assert(parent != nullptr);

			models.resize(count);
			for (auto tcIdx = oldSize; tcIdx < count; ++tcIdx)
			{
				auto& channels = models[tcIdx];
				channels.reserve(pluginChannels);
				for (int pcIdx = 0; pcIdx < pluginChannels; ++pcIdx)
				{
					const auto name = nameFormat.arg(tcIdx + 1).arg(matrix.channelName(pcIdx));
					BoolModel* model = channels.emplace_back(new BoolModel{false, parent, name});
					connect(model, &BoolModel::dataChanged, this, &PluginPinConnector::dataChanged);
				}
			}
		}
	};

	updateModels(in().channelCount, QString::fromUtf16(u"Pin in [%1 \U0001F82E %2]"), m_in);
	updateModels(out().channelCount, QString::fromUtf16(u"Pin out [%2 \U0001F82E %1]"), m_out);

	emit propertiesChanged();
}

} // namespace lmms
