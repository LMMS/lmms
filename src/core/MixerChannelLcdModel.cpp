/*
 * MixerChannelModel.cpp
 *
 * Copyright (c) 2026 saker <sakertooth@gmail.com>
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

#include "MixerChannelLcdModel.h"

#include "Engine.h"
#include "Mixer.h"

namespace lmms {
MixerChannelLcdModel::MixerChannelLcdModel(Model* parent)
	: IntModel(0, 0, 0, parent, tr("Mixer channel"))
{
	const auto mixer = Engine::mixer();
	setRange(0, mixer->numChannels() - 1, 1);

	connect(mixer, &Mixer::channelsSwapped, this, &MixerChannelLcdModel::channelsSwapped);
	connect(mixer, &Mixer::channelDeleted, this, &MixerChannelLcdModel::channelDeleted);
	connect(mixer, &Mixer::channelCreated, this, &MixerChannelLcdModel::channelCreated);

	connect(this, &MixerChannelLcdModel::dataChanged, mixer, [this, mixer] {
		// both channels must exist so we know the channel was actually changed to another
		if (oldValue() < 0 || oldValue() >= mixer->numChannels()) { return; }
		if (value() < 0 || value() >= mixer->numChannels()) { return; }

		mixer->mixerChannel(oldValue())->decrementUseCount();
		mixer->mixerChannel(value())->incrementUseCount();
	});

	mixer->mixerChannel(0)->incrementUseCount();
}

MixerChannelLcdModel::~MixerChannelLcdModel()
{
	const auto mixer = Engine::mixer();
	if (value() < 0 || value() >= mixer->numChannels()) { return; }
	mixer->mixerChannel(value())->decrementUseCount();
}

void MixerChannelLcdModel::channelsSwapped(int fromIndex, int toIndex)
{
	if (value() == fromIndex) { setValue(toIndex); }
	else if (value() == toIndex) { setValue(fromIndex); }
}

void MixerChannelLcdModel::channelDeleted(int index)
{
	if (value() == index) { setValue(0); }
	else if (value() > index) { setValue(value() - 1); }
	setRange(0, maxValue() - 1);
}

void MixerChannelLcdModel::channelCreated(int index)
{
	setRange(0, maxValue() + 1);
}

} // namespace lmms
