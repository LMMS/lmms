/*
 * AudioPortsModel.cpp - The model for audio ports used by the
 *                       pin connector
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

#include "AudioPortsModel.h"

#include <QDomDocument>
#include <QDomElement>
#include <QDebug>
#include <stdexcept>

#include "AudioBufferView.h"
#include "AudioEngine.h"
#include "Engine.h"
#include "PinConnector.h"

namespace lmms
{

AudioPortsModel::AudioPortsModel(bool isInstrument, Model* parent)
	: AudioPortsModel{DynamicChannelCount, DynamicChannelCount, isInstrument, parent}
{
}

AudioPortsModel::AudioPortsModel(proc_ch_t channelCountIn, proc_ch_t channelCountOut, bool isInstrument, Model* parent)
	: Model{parent}
	, m_isInstrument{isInstrument}
{
	setTrackChannelCountImpl(DEFAULT_CHANNELS); // TODO: Will be >=2 once support for additional track channels is added
	setProcessorChannelCountsImpl(channelCountIn, channelCountOut);

	updateDirectRouting();
	updateAllUsedChannels();

	connect(Engine::audioEngine(), &AudioEngine::sampleRateChanged, [this]() {
		bufferPropertiesChanged(in().channelCount(), out().channelCount(), Engine::audioEngine()->framesPerPeriod());
	});
}

void AudioPortsModel::setAllChannelCounts(track_ch_t trackChannels, proc_ch_t inCount, proc_ch_t outCount)
{
	const auto oldTrackChannelCount = m_totalTrackChannels;
	const auto oldOutCount = m_out.channelCount();

	const bool tcChanged = setTrackChannelCountImpl(trackChannels);
	const bool pcChanged = setProcessorChannelCountsImpl(inCount, outCount);
	if (!tcChanged && !pcChanged) { return; }

	// Now update the cache
	updateDirectRouting();
	if (m_totalTrackChannels < oldTrackChannelCount && m_out.channelCount() < oldOutCount)
	{
		updateAllUsedChannels();
	}
	else if (m_totalTrackChannels < oldTrackChannelCount)
	{
		updateAllUsedProcessorChannels();
	}
	else if (m_out.channelCount() < oldOutCount)
	{
		updateAllUsedTrackChannels();
	}

	if (pcChanged)
	{
		// Now tell the audio buffer to update
		bufferPropertiesChanged(inCount, outCount, Engine::audioEngine()->framesPerPeriod());
	}

	emit propertiesChanged();
}

void AudioPortsModel::setTrackChannelCount(track_ch_t count)
{
	const auto oldTrackChannelCount = m_totalTrackChannels;

	if (!setTrackChannelCountImpl(count)) { return; }

	// Now update the cache
	updateDirectRouting();
	if (m_totalTrackChannels < oldTrackChannelCount)
	{
		updateAllUsedProcessorChannels();
	}

	emit propertiesChanged();
}

void AudioPortsModel::setChannelCounts(proc_ch_t inCount, proc_ch_t outCount)
{
	const auto oldOutCount = m_out.channelCount();

	if (!setProcessorChannelCountsImpl(inCount, outCount)) { return; }

	// Now update the cache
	updateDirectRouting();
	if (m_out.channelCount() < oldOutCount)
	{
		updateAllUsedTrackChannels();
	}

	// Now tell the audio buffer to update
	bufferPropertiesChanged(inCount, outCount, Engine::audioEngine()->framesPerPeriod());

	emit propertiesChanged();
}

auto AudioPortsModel::setTrackChannelCountImpl(track_ch_t count) -> bool
{
	if (count < 2) { throw std::invalid_argument{"There must be at least 2 track channels"}; }
	if (count % 2 != 0) { throw std::invalid_argument{"There must be an even number of track channels"}; }

	if (count > MaxTrackChannels)
	{
		throw std::invalid_argument{"Only up to 256 track channels are allowed"};
	}

	if (in().pins().size() == count && out().pins().size() == count)
	{
		return false;
	}

	m_trackChannelsUpperBound = std::min(m_trackChannelsUpperBound, count);
	m_usedTrackChannels.resize(count);
	m_totalTrackChannels = count;

	m_in.setTrackChannelCount(count);
	m_out.setTrackChannelCount(count);

	return true;
}

auto AudioPortsModel::setProcessorChannelCountsImpl(proc_ch_t inCount, proc_ch_t outCount) -> bool
{
	if (inCount == DynamicChannelCount || outCount == DynamicChannelCount)
	{
		return false;
	}

	if (inCount == 0 && outCount == 0)
	{
		qWarning() << "At least one port count must be non-zero";
		return false;
	}

	if (in().channelCount() == inCount && out().channelCount() == outCount)
	{
		// No action needed
		return false;
	}

	m_usedProcessorChannels.resize(outCount);

	m_in.setChannelCount(inCount);
	m_out.setChannelCount(outCount);

	return true;
}

void AudioPortsModel::setChannelCountIn(proc_ch_t inCount)
{
	setChannelCounts(inCount, out().channelCount());
}

void AudioPortsModel::setChannelCountOut(proc_ch_t outCount)
{
	setChannelCounts(in().channelCount(), outCount);
}

void AudioPortsModel::saveSettings(QDomDocument& doc, QDomElement& elem)
{
	auto pins = doc.createElement(nodeName());
	elem.appendChild(pins);

	pins.setAttribute("inputs", in().channelCount());
	pins.setAttribute("outputs", out().channelCount());

	auto pinsIn = doc.createElement("in_matrix");
	pins.appendChild(pinsIn);
	m_in.saveSettings(doc, pinsIn);

	auto pinsOut = doc.createElement("out_matrix");
	pins.appendChild(pinsOut);
	m_out.saveSettings(doc, pinsOut);
}

void AudioPortsModel::loadSettings(const QDomElement& elem)
{
	const auto pins = elem.firstChildElement(nodeName());
	if (pins.isNull()) { return; }

	// TODO: Assert port counts are what was expected?
	const auto inputs = pins.attribute("inputs", "0").toInt();
	const auto outputs = pins.attribute("outputs", "0").toInt();
	setChannelCounts(inputs, outputs);

	m_in.loadSettings(pins.firstChildElement("in_matrix"));
	m_out.loadSettings(pins.firstChildElement("out_matrix"));

	updateAllUsedChannels();
	updateDirectRouting();

	emit dataChanged();
}

void AudioPortsModel::updateUsedTrackChannels(track_ch_t trackChannel)
{
	const auto& pins = m_out.m_pins.at(trackChannel);
	m_usedTrackChannels[trackChannel] = std::any_of(pins.begin(), pins.end(), [](bool m) {
		return m;
	});
}

void AudioPortsModel::updateUsedProcessorChannels(proc_ch_t outChannel)
{
	const auto& pins = m_out.m_pins;
	m_usedProcessorChannels[outChannel] = std::any_of(pins.begin(), pins.end(), [=](const auto& p) {
		return p[outChannel];
	});
}

void AudioPortsModel::updateAllUsedChannels()
{
	std::fill(m_usedProcessorChannels.begin(), m_usedProcessorChannels.end(), false);

	for (track_ch_t tc = 0; tc < m_totalTrackChannels; ++tc)
	{
		m_usedTrackChannels[tc] = false;
		for (proc_ch_t pc = 0; pc < m_out.channelCount(); ++pc)
		{
			const bool enabled = m_out.enabled(tc, pc);
			m_usedTrackChannels[tc] = m_usedTrackChannels[tc] || enabled;
			m_usedProcessorChannels[pc] = m_usedProcessorChannels[pc] || enabled;
		}
	}
}

void AudioPortsModel::updateAllUsedTrackChannels()
{
	for (track_ch_t tc = 0; tc < m_totalTrackChannels; ++tc)
	{
		updateUsedTrackChannels(tc);
	}
}

void AudioPortsModel::updateAllUsedProcessorChannels()
{
	std::fill(m_usedProcessorChannels.begin(), m_usedProcessorChannels.end(), false);

	for (track_ch_t tc = 0; tc < m_totalTrackChannels; ++tc)
	{
		for (proc_ch_t pc = 0; pc < m_out.channelCount(); ++pc)
		{
			m_usedProcessorChannels[pc] = m_usedProcessorChannels[pc] || m_out.enabled(tc, pc);
		}
	}
}

void AudioPortsModel::updateDirectRouting()
{
	const auto ins = in().channelCount();
	const auto outs = out().channelCount();

	if (ins == 0 && outs == 0)
	{
		m_directRouting.reset();
		return;
	}

	if ((ins != 0 && ins != 2) || (outs != 0 && outs != 2))
	{
		// If one of the inputs or outputs is not 2 channels (or nonexistent),
		// the optimization is impossible
		m_directRouting.reset();
		return;
	}

	// In order for the optimization to be possible, the pin connections must look
	// like this for all ports with > 0 channels for exactly one track channel pair:
	//  ___
	// |X| |
	// | |X|
	//  ---

	auto check = [&, this](const Matrix& m) -> std::optional<track_ch_t> {
		auto ret = std::optional<track_ch_t>{};
		int nonzeroCount = 0;
		for (track_ch_t tc = 0; tc < trackChannelCount(); tc += 2)
		{
			// Check for any enabled pins
			if (m.enabled(tc, 0) || m.enabled(tc, 1) || m.enabled(tc + 1, 0) || m.enabled(tc + 1, 1))
			{
				++nonzeroCount;
				if (nonzeroCount > 1)
				{
					// More than one track channel pair is connected to/from
					// the processor channels, so the optimization is impossible
					return std::nullopt;
				}
			}

			// Check for direct connection
			if (!ret && m.enabled(tc, 0) && !m.enabled(tc, 1) && !m.enabled(tc + 1, 0) && m.enabled(tc + 1, 1))
			{
				ret = tc / 2;
			}
		}

		return ret;
	};

	if (ins == 0)
	{
		m_directRouting = check(out());
	}
	else if (outs == 0)
	{
		m_directRouting = check(in());
	}
	else
	{
		// 2x2 audio processor
		auto inOpt = check(in());
		if (!inOpt)
		{
			// The optimization cannot be applied to the input matrix
			m_directRouting.reset();
			return;
		}

		auto outOpt = check(out());
		if (!outOpt)
		{
			// The optimization cannot be applied to the output matrix
			m_directRouting.reset();
			return;
		}

		if (inOpt != outOpt)
		{
			// This optimization only supports in-place processing for now,
			// so the same track channel pair must be used on the input and output.
			m_directRouting.reset();
			return;
		}

		m_directRouting = inOpt;
	}
}

auto AudioPortsModel::instantiateView() const -> gui::PinConnector*
{
	// This method does not modify AudioPortsModel, but it needs PinConnector to store
	// a mutable pointer to the AudioPortsModel, hence the const_cast.
	return new gui::PinConnector{const_cast<AudioPortsModel*>(this)};
}

auto AudioPortsModel::getChannelCountText() const -> QString
{
	const auto inText = QString{"%1"}.arg(in().channelCount());
	const auto outText = QString{"%1"}.arg(out().channelCount());
	return QString{tr("%1 in %2 out")}.arg(inText).arg(outText);
}

auto AudioPortsModel::channelName(proc_ch_t channel, bool isOutput) const -> QString
{
	return isOutput
		? tr("Output %1").arg(channel + 1)
		: tr("Input %1").arg(channel + 1);
}

auto AudioPortsModel::Matrix::channelName(proc_ch_t channel) const -> QString
{
	return m_parent->channelName(channel, isOutput());
}

void AudioPortsModel::Matrix::setPin(track_ch_t trackChannel, proc_ch_t processorChannel, bool value)
{
	const bool oldValue = m_pins[trackChannel][processorChannel];
	m_pins[trackChannel][processorChannel] = value;

	updateCache(trackChannel, processorChannel);
	if (value != oldValue)
	{
		emit m_parent->dataChanged();
	}
}

void AudioPortsModel::Matrix::updateCache(track_ch_t trackChannel, proc_ch_t processorChannel)
{
	if (isOutput())
	{
		m_parent->updateUsedTrackChannels(trackChannel);
		m_parent->updateUsedProcessorChannels(processorChannel);
	}
	m_parent->updateDirectRouting();
}

void AudioPortsModel::Matrix::setTrackChannelCount(track_ch_t count)
{
	auto oldSize = static_cast<track_ch_t>(m_pins.size());
	if (oldSize > count)
	{
		m_pins.resize(count);
	}
	else if (oldSize < count)
	{
		m_pins.resize(count);
		for (auto tcIdx = oldSize; tcIdx < count; ++tcIdx)
		{
			auto& channels = m_pins[tcIdx];
			channels.reserve(m_channelCount);
			for (proc_ch_t pcIdx = 0; pcIdx < m_channelCount; ++pcIdx)
			{
				channels.emplace_back(false);
			}
		}
	}
}

void AudioPortsModel::Matrix::setChannelCount(proc_ch_t count)
{
	const bool initialSetup = m_channelCount == 0;

	if (channelCount() < count)
	{
		for (track_ch_t tcIdx = 0; tcIdx < m_pins.size(); ++tcIdx)
		{
			auto& processorChannels = m_pins[tcIdx];
			processorChannels.reserve(count);
			for (proc_ch_t pcIdx = channelCount(); pcIdx < count; ++pcIdx)
			{
				processorChannels.emplace_back(false);
			}
		}
	}
	else if (channelCount() > count)
	{
		for (auto& processorChannels : m_pins)
		{
			processorChannels.erase(processorChannels.begin() + count, processorChannels.end());
		}
	}

	m_channelCount = count;

	if (initialSetup)
	{
		setDefaultConnections();
	}
}

void AudioPortsModel::Matrix::setDefaultConnections()
{
	assert(m_pins.size() >= 2u);

	if (m_parent->isInstrument() && !isOutput())
	{
		// Don't set the input matrix for an instrument
		return;
	}

	switch (channelCount())
	{
		case 0: break;
		case 1: // mono
			setPin(0, 0, true);
			if (isOutput())
			{
				// Only the first track channel is routed to mono-input audio
				// processors, otherwise the pin connector's input-summing behavior
				// would cause mono processors to be louder than stereo ones.
				// This behavior matches what REAPER's plug-in pin connector does.
				setPin(1, 0, true);
			}
			break;
		default: // >= 2
			setPin(0, 0, true);
			setPin(1, 1, true);
			break;
	}
}

void AudioPortsModel::Matrix::saveSettings(QDomDocument& doc, QDomElement& elem) const
{
	// Only saves connections that are actually used, otherwise could bloat project file
	for (std::size_t trackChannel = 0; trackChannel < m_pins.size(); ++trackChannel)
	{
		auto& processorChannels = m_pins[trackChannel];
		for (std::size_t processorChannel = 0; processorChannel < processorChannels.size(); ++processorChannel)
		{
			if (processorChannels[processorChannel])
			{
				const auto name = QStringLiteral("c%1_%2").arg(trackChannel + 1).arg(processorChannel + 1);
				assert(!AutomatableModel::mustQuoteName(name));

				elem.setAttribute(name, true);
			}
		}
	}
}

void AudioPortsModel::Matrix::loadSettings(const QDomElement& elem)
{
	assert(m_channelCount == (trackChannelCount() > 0 ? static_cast<proc_ch_t>(m_pins[0].size()) : 0));

	// Initialize all pins to OFF
	for (auto& processorChannels : m_pins)
	{
		std::fill(processorChannels.begin(), processorChannels.end(), false);
	}

	auto addConnection = [&](const QString& name) {
		const auto pos = name.indexOf('_');
#ifndef NDEBUG
		constexpr auto minSize = static_cast<int>(std::string_view{"c#_#"}.size());
		if (name.size() < minSize) { throw std::runtime_error{"string too small"}; }
		if (name[0] != 'c') { throw std::runtime_error{"invalid string: \"" + name.toStdString() + "\""}; }
		if (pos <= 0) { throw std::runtime_error{"parse failure"}; }
#endif

		auto trackChannel = name.midRef(1, pos - 1).toInt();
		auto processorChannel = name.midRef(pos + 1).toInt();
#ifndef NDEBUG
		if (trackChannel == 0 || processorChannel == 0) { throw std::runtime_error{"failed to parse integer"}; }
#endif

		setPinBatch(trackChannel - 1, processorChannel - 1, true);
	};

	// Get pin connector connections
	const auto& attrs = elem.attributes();
	for (int idx = 0; idx < attrs.size(); ++idx)
	{
		const auto node = attrs.item(idx);
		addConnection(node.nodeName());
	}
}

} // namespace lmms
