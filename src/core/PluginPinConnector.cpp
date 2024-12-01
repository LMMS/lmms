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
#include "PluginPinConnectorView.h"

namespace lmms
{

namespace
{

// Scratch pad for intermediate calculations within this class
thread_local auto WorkingBuffer = std::array<float, MAXIMUM_BUFFER_SIZE>();

} // namespace

PluginPinConnector::PluginPinConnector(Model* parent)
	: Model{parent}
{
	setTrackChannelCount(s_totalTrackChannels);
}

PluginPinConnector::PluginPinConnector(int pluginChannelCountIn, int pluginChannelCountOut, Model* parent)
	: Model{parent}
{
	setTrackChannelCount(s_totalTrackChannels);
	setPluginChannelCounts(pluginChannelCountIn, pluginChannelCountOut);
}

void PluginPinConnector::setPluginChannelCounts(int inCount, int outCount)
{
	if (m_trackChannelsUpperBound > MaxTrackChannels)
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

	if (in().channelCount() == inCount && out().channelCount() == outCount)
	{
		// No action needed
		return;
	}

	const bool initialSetup = in().channelCount() == 0 && out().channelCount() == 0;

	m_in.setPluginChannelCount(this, inCount, QString::fromUtf16(u"Pin in [%1 \U0001F82E %2]"));
	m_out.setPluginChannelCount(this, outCount, QString::fromUtf16(u"Pin out [%2 \U0001F82E %1]"));

	if (initialSetup)
	{
		setDefaultConnections();
	}

	emit propertiesChanged();
}

void PluginPinConnector::setPluginChannelCountIn(int inCount)
{
	setPluginChannelCounts(inCount, out().channelCount());
}

void PluginPinConnector::setPluginChannelCountOut(int outCount)
{
	setPluginChannelCounts(in().channelCount(), outCount);
}

void PluginPinConnector::setDefaultConnections()
{
	// Assumes pin maps are cleared
	// TODO: Take into account channel groups?

	m_in.setDefaultConnections();
	m_out.setDefaultConnections();
}

void PluginPinConnector::routeToPlugin(f_cnt_t frames,
	CoreAudioBufferView in, SplitAudioBufferView<sample_t> out) const
{
	// Ignore all unused track channels for better performance
	const auto inSizeConstrained = m_trackChannelsUpperBound / 2;
	assert(inSizeConstrained <= in.size());

	// Zero the output buffer
	std::fill(out.begin(), out.end(), 0.f);

	std::uint8_t outChannel = 0;
	for (f_cnt_t outSampleIdx = 0; outSampleIdx < out.size(); outSampleIdx += frames, ++outChannel)
	{
		mix_ch_t numRouted = 0; // counter for # of in channels routed to the current out channel
		SplitSampleType<sample_t>* outPtr = &out[outSampleIdx];

		for (std::uint8_t inChannelPairIdx = 0; inChannelPairIdx < inSizeConstrained; ++inChannelPairIdx)
		{
			const SampleFrame* inPtr = in[inChannelPairIdx]; // L/R track channel pair

			const std::uint8_t inChannel = inChannelPairIdx * 2;
			const std::uint8_t enabledPins =
				(static_cast<std::uint8_t>(m_in.enabled(inChannel, outChannel)) << 1u)
				| static_cast<std::uint8_t>(m_in.enabled(inChannel + 1, outChannel));

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

		// Either no input channels were routed to this output and output stays zeroed,
		// or only one channel was routed and normalization is not needed
		if (numRouted <= 1) { continue; }

		// Normalize output
		for (f_cnt_t frame = 0; frame < frames; ++frame)
		{
			outPtr[frame] /= numRouted;
		}
	}
}

void PluginPinConnector::routeFromPlugin(f_cnt_t frames,
	SplitAudioBufferView<const sample_t> in, CoreAudioBufferViewMut inOut) const
{
	assert(frames <= MAXIMUM_BUFFER_SIZE);

	// Ignore all unused track channels for better performance
	const auto inOutSizeConstrained = m_trackChannelsUpperBound / 2;
	assert(inOutSizeConstrained <= inOut.size());

	for (std::uint8_t outChannelPairIdx = 0; outChannelPairIdx < inOutSizeConstrained; ++outChannelPairIdx)
	{
		SampleFrame* outPtr = inOut[outChannelPairIdx]; // L/R track channel pair
		const auto outChannel = static_cast<std::uint8_t>(outChannelPairIdx * 2);

		// TODO C++20: Use explicit non-type template parameter instead of `outChannelOffset` auto parameter
		const auto mixInputs = [&](std::uint8_t outChannel, auto outChannelOffset) {
			constexpr auto outChannelOffsetConst = outChannelOffset();
			WorkingBuffer.fill(0.f); // used as buffer out

			// Counter for # of in channels routed to the current out channel
			mix_ch_t numRouted = 0;

			std::uint8_t inChannel = 0;
			for (f_cnt_t inSampleIdx = 0; inSampleIdx < in.size(); inSampleIdx += frames, ++inChannel)
			{
				if (!m_out.enabled(outChannel + outChannelOffsetConst, inChannel)) { continue; }

				const SplitSampleType<sample_t>* inPtr = &in[inSampleIdx];
				for (f_cnt_t frame = 0; frame < frames; ++frame)
				{
					WorkingBuffer[frame] += inPtr[frame];
				}
				++numRouted;
			}

			switch (numRouted)
			{
				case 0:
					// Nothing needs to be written to `inOut` for audio bypass,
					// since it already contains the LMMS core input audio.
					break;
				case 1:
				{
					// Normalization not needed, but copying is
					for (f_cnt_t frame = 0; frame < frames; ++frame)
					{
						outPtr[frame][outChannelOffsetConst] = WorkingBuffer[frame];
					}
					break;
				}
				default: // >= 2
				{
					// Normalize output
					for (f_cnt_t frame = 0; frame < frames; ++frame)
					{
						outPtr[frame][outChannelOffsetConst] = WorkingBuffer[frame] / numRouted;
					}
					break;
				}
			}
		};

		// Left SampleFrame channel first
		mixInputs(outChannel, std::integral_constant<int, 0>{});

		// Right SampleFrame channel second
		mixInputs(outChannel, std::integral_constant<int, 1>{});
	}
}

void PluginPinConnector::saveSettings(QDomDocument& doc, QDomElement& elem)
{
	auto pins = doc.createElement(nodeName());
	elem.appendChild(pins);

	if (m_trackChannelsUpperBound != s_totalTrackChannels)
	{
		pins.setAttribute("tc_used", m_trackChannelsUpperBound);
	}

	pins.setAttribute("num_in", in().channelCount());
	pins.setAttribute("num_out", out().channelCount());

	auto pinsIn = doc.createElement("in_matrix");
	pins.appendChild(pinsIn);
	m_in.saveSettings(doc, pinsIn);

	auto pinsOut = doc.createElement("out_matrix");
	pins.appendChild(pinsOut);
	m_out.saveSettings(doc, pinsOut);
}

void PluginPinConnector::loadSettings(const QDomElement& elem)
{
	const auto pins = elem.firstChildElement(nodeName());
	if (pins.isNull()) { return; }

	// Until full routing support is added, track channel count should always be 2
	m_trackChannelsUpperBound = pins.attribute("tc_used", QString::number(s_totalTrackChannels)).toUInt(); // TODO: Calculate m_trackChannelsUpperBound instead
	assert(m_trackChannelsUpperBound == 2);

	// TODO: Assert port counts are what was expected?
	const auto pluginInCount = pins.attribute("num_in", "0").toInt();
	const auto pluginOutCount = pins.attribute("num_out", "0").toInt();
	setPluginChannelCounts(pluginInCount, pluginOutCount);

	m_in.loadSettings(pins.firstChildElement("in_matrix"));
	m_out.loadSettings(pins.firstChildElement("out_matrix"));
}

void PluginPinConnector::setTrackChannelCount(int count)
{
	if (count < 2) { throw std::invalid_argument{"There must be at least 2 track channels"}; }
	if (count % 2 != 0) { throw std::invalid_argument{"There must be an even number of track channels"}; }

	if (static_cast<int>(in().pins().size()) == count
		&& static_cast<int>(out().pins().size()) == count)
	{
		return;
	}

	m_trackChannelsUpperBound = std::min<unsigned>(m_trackChannelsUpperBound, count);

	m_in.setTrackChannelCount(this, count, QString::fromUtf16(u"Pin in [%1 \U0001F82E %2]"));
	m_out.setTrackChannelCount(this, count, QString::fromUtf16(u"Pin out [%2 \U0001F82E %1]"));

	emit propertiesChanged();
}

auto PluginPinConnector::instantiateView(QWidget* parent) -> gui::PluginPinConnectorView*
{
	return new gui::PluginPinConnectorView{this, parent};
}

auto PluginPinConnector::getChannelCountText() const -> QString
{
	const auto inText = QString{"%1"}.arg(in().channelCount());
	const auto outText = QString{"%1"}.arg(out().channelCount());
	return QString{tr("%1 in %2 out")}.arg(inText).arg(outText);
}

auto PluginPinConnector::Matrix::channelName(int channel) const -> QString
{
	// Custom name (if supported)
	assert(channel >= 0);
	if (channel < static_cast<int>(m_channelNames.size()))
	{
		return m_channelNames[channel];
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

void PluginPinConnector::Matrix::setTrackChannelCount(PluginPinConnector* parent, int count,
	const QString& nameFormat)
{
	auto oldSize = static_cast<int>(m_pins.size());
	if (oldSize > count)
	{
		for (auto tcIdx = count; tcIdx < oldSize; ++tcIdx)
		{
			for (BoolModel* model : m_pins[tcIdx])
			{
				delete model;
			}
		}

		m_pins.resize(count);
	}
	else if (oldSize < count)
	{
		auto parentModel = dynamic_cast<Model*>(parent->parent());
		assert(parentModel != nullptr);

		m_pins.resize(count);
		for (auto tcIdx = oldSize; tcIdx < count; ++tcIdx)
		{
			auto& channels = m_pins[tcIdx];
			channels.reserve(m_channelCount);
			for (int pcIdx = 0; pcIdx < m_channelCount; ++pcIdx)
			{
				const auto name = nameFormat.arg(tcIdx + 1).arg(channelName(pcIdx));
				BoolModel* model = channels.emplace_back(new BoolModel{false, parentModel, name});
				connect(model, &BoolModel::dataChanged, parent, &PluginPinConnector::dataChanged);
			}
		}
	}
}

void PluginPinConnector::Matrix::setPluginChannelCount(PluginPinConnector* parent, int count,
	const QString& nameFormat)
{
	auto parentModel = dynamic_cast<Model*>(parent->parent());
	assert(parentModel != nullptr);

	if (channelCount() < count)
	{
		for (unsigned tcIdx = 0; tcIdx < m_pins.size(); ++tcIdx)
		{
			auto& pluginChannels = m_pins[tcIdx];
			pluginChannels.reserve(count);
			for (int pcIdx = channelCount(); pcIdx < count; ++pcIdx)
			{
				const auto name = nameFormat.arg(tcIdx + 1).arg(channelName(pcIdx));
				BoolModel* model = pluginChannels.emplace_back(new BoolModel{false, parentModel, name});
				connect(model, &BoolModel::dataChanged, parent, &PluginPinConnector::dataChanged);
			}
		}
	}
	else if (channelCount() > count)
	{
		for (auto& pluginChannels : m_pins)
		{
			for (int pcIdx = count; pcIdx < channelCount(); ++pcIdx)
			{
				delete pluginChannels[pcIdx];
			}
			pluginChannels.erase(pluginChannels.begin() + count, pluginChannels.end());
		}
	}

	m_channelCount = count;
}

void PluginPinConnector::Matrix::setDefaultConnections()
{
	assert(m_pins.size() >= 2u);

	switch (channelCount())
	{
		case 0: break;
		case 1:
			m_pins[0][0]->setValue(true);
			m_pins[1][0]->setValue(true);
			break;
		default: // >= 2
			m_pins[0][0]->setValue(true);
			m_pins[1][1]->setValue(true);
			break;
	}
}

void PluginPinConnector::Matrix::saveSettings(QDomDocument& doc, QDomElement& elem) const
{
	// Only saves connections that are actually used, otherwise could bloat project file
	for (std::size_t trackChannel = 0; trackChannel < m_pins.size(); ++trackChannel)
	{
		auto& trackChannels = m_pins[trackChannel];
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

void PluginPinConnector::Matrix::loadSettings(const QDomElement& elem)
{
	const auto trackChannelCount = static_cast<int>(m_pins.size());
	assert(m_channelCount == (trackChannelCount > 0 ? static_cast<int>(m_pins[0].size()) : 0));

	std::vector<std::vector<bool>> connectionsToLoad;
	connectionsToLoad.resize(trackChannelCount);
	for (auto& pluginChannels : connectionsToLoad)
	{
		pluginChannels.resize(m_channelCount);
	}

	auto addConnection = [&connectionsToLoad](const QString& name) {
		const auto pos = name.indexOf('_');
#ifndef NDEBUG
		constexpr auto minSize = static_cast<int>(std::string_view{"c#_#"}.size());
		if (name.size() < minSize) { throw std::runtime_error{"string too small"}; }
		if (name[0] != 'c') { throw std::runtime_error{"invalid string: \"" + name.toStdString() + "\""}; }
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
	for (std::size_t trackChannel = 0; trackChannel < m_pins.size(); ++trackChannel)
	{
		auto& trackChannels = m_pins[trackChannel];
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

} // namespace lmms
