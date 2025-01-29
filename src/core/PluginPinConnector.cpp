/*
 * PluginPinConnector.cpp - Specifies how to route audio channels
 *                          in and out of a plugin.
 *
 * Copyright (c) 2025 Dalton Messmer <messmer.dalton/at/gmail.com>
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
#include <QDebug>
#include <stdexcept>

#include "AudioEngine.h"
#include "Engine.h"
#include "PluginPinConnectorView.h"

namespace lmms
{

PluginPinConnector::PluginPinConnector(bool isInstrument, Model* parent)
	: Model{parent}
	, m_isInstrument{isInstrument}
{
	setTrackChannelCount(s_totalTrackChannels);

	connect(Engine::audioEngine(), &AudioEngine::sampleRateChanged, [this]() {
		bufferPropertiesChanged(in().channelCount(), out().channelCount(), Engine::audioEngine()->framesPerPeriod());
	});
}

PluginPinConnector::PluginPinConnector(int pluginChannelCountIn, int pluginChannelCountOut, bool isInstrument, Model* parent)
	: Model{parent}
	, m_isInstrument{isInstrument}
{
	setTrackChannelCount(s_totalTrackChannels);
	setPluginChannelCountsImpl(pluginChannelCountIn, pluginChannelCountOut);

	connect(Engine::audioEngine(), &AudioEngine::sampleRateChanged, [this]() {
		bufferPropertiesChanged(in().channelCount(), out().channelCount(), Engine::audioEngine()->framesPerPeriod());
	});
}

void PluginPinConnector::setPluginChannelCounts(int inCount, int outCount)
{
	setPluginChannelCountsImpl(inCount, outCount);

	// Now tell the audio buffer to update
	bufferPropertiesChanged(inCount, outCount, Engine::audioEngine()->framesPerPeriod());

	emit propertiesChanged();
}

void PluginPinConnector::setPluginChannelCountsImpl(int inCount, int outCount)
{
	if (m_trackChannelsUpperBound > MaxTrackChannels)
	{
		throw std::runtime_error{"Only up to 256 track channels are allowed"};
	}

	if (inCount == DynamicChannelCount || outCount == DynamicChannelCount)
	{
		return;
	}

	if (inCount < 0)
	{
		qWarning() << "Invalid input count";
		return;
	}

	if (outCount < 0)
	{
		qWarning() << "Invalid output count";
		return;
	}

	if (inCount == 0 && outCount == 0)
	{
		qWarning() << "At least one port count must be non-zero";
		return;
	}

	if (in().channelCount() == inCount && out().channelCount() == outCount)
	{
		// No action needed
		return;
	}

	m_in.setPluginChannelCount(this, inCount, QString::fromUtf16(u"Pin in [%1 \U0001F82E %2]"));
	m_out.setPluginChannelCount(this, outCount, QString::fromUtf16(u"Pin out [%2 \U0001F82E %1]"));
}

void PluginPinConnector::setPluginChannelCountIn(int inCount)
{
	setPluginChannelCounts(inCount, out().channelCount());
}

void PluginPinConnector::setPluginChannelCountOut(int outCount)
{
	setPluginChannelCounts(in().channelCount(), outCount);
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
	m_routedChannels.resize(static_cast<std::size_t>(count));

	m_in.setTrackChannelCount(this, count, QString::fromUtf16(u"Pin in [%1 \U0001F82E %2]"));
	m_out.setTrackChannelCount(this, count, QString::fromUtf16(u"Pin out [%2 \U0001F82E %1]"));

	updateAllRoutedChannels();

	emit propertiesChanged();
}

void PluginPinConnector::updateRoutedChannels(unsigned int trackChannel)
{
	const auto& pins = m_out.m_pins.at(trackChannel);
	m_routedChannels[trackChannel] = std::any_of(pins.begin(), pins.end(), [](BoolModel* m) {
		return m->value();
	});
}

void PluginPinConnector::updateAllRoutedChannels()
{
	for (unsigned int tc = 0; tc < s_totalTrackChannels; ++tc)
	{
		updateRoutedChannels(tc);
	}
}

auto PluginPinConnector::instantiateView(QWidget* parent) const -> gui::PluginPinConnectorView*
{
	// This method does not modify the pin connector, but it needs the view to store
	// a mutable pointer to the pin connector, hence the const_cast.
	return new gui::PluginPinConnectorView{const_cast<PluginPinConnector*>(this), parent};
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
		auto parentModel = parent->parentModel();
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
				if (isOutput())
				{
					parentModel->connect(model, &BoolModel::dataChanged, [=]() { parent->updateRoutedChannels(tcIdx); });
				}
				parentModel->connect(model, &BoolModel::dataChanged, parent, &PluginPinConnector::dataChanged);
			}
		}
	}
}

void PluginPinConnector::Matrix::setPluginChannelCount(PluginPinConnector* parent, int count,
	const QString& nameFormat)
{
	auto parentModel = parent->parentModel();
	assert(parentModel != nullptr);

	const bool initialSetup = m_channelCount == 0;

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
				if (isOutput())
				{
					parentModel->connect(model, &BoolModel::dataChanged, [=]() { parent->updateRoutedChannels(tcIdx); });
				}
				parentModel->connect(model, &BoolModel::dataChanged, parent, &PluginPinConnector::dataChanged);
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

	if (initialSetup && (!parent->isInstrument() || isOutput()))
	{
		// Set default connections, unless this is the input matrix for an instrument
		setDefaultConnections();
	}
}

void PluginPinConnector::Matrix::setDefaultConnections()
{
	assert(m_pins.size() >= 2u);

	switch (channelCount())
	{
		case 0: break;
		case 1: // mono
			m_pins[0][0]->setValue(true);
			if (isOutput())
			{
				// Only the first track channel is routed to mono-input
				// plugins, otherwise the pin connector's input-summing behavior
				// would cause mono plugins to be louder than stereo ones.
				// This behavior matches what REAPER's plug-in pin connector does.
				m_pins[1][0]->setValue(true);
			}
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
