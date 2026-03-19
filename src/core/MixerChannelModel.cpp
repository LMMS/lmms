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

#include "MixerChannelModel.h"

#include "Engine.h"
#include "Mixer.h"

namespace lmms {
MixerChannelModel::MixerChannelModel(Model* parent)
	: IntModel(0, 0, 0, parent, tr("Mixer channel"))
{
	setRange(0, Engine::mixer()->numChannels() - 1, 1);

	connect(Engine::mixer(), &Mixer::channelsSwapped, this, &MixerChannelModel::channelsSwapped);
	connect(Engine::mixer(), &Mixer::channelDeleted, this, &MixerChannelModel::channelDeleted);
	connect(Engine::mixer(), &Mixer::channelCreated, this, &MixerChannelModel::channelCreated);

	connect(this, &MixerChannelModel::dataChanged, Engine::mixer(), [this] {
		--Engine::mixer()->mixerChannel(oldValue())->m_useCount;
		++Engine::mixer()->mixerChannel(value())->m_useCount;
	});

	++Engine::mixer()->mixerChannel(0)->m_useCount;
}

void MixerChannelModel::channelsSwapped(int fromIndex, int toIndex)
{
	if (value() == fromIndex) { setValue(toIndex); }
	else if (value() == toIndex) { setValue(fromIndex); }
}

void MixerChannelModel::channelDeleted(int index)
{
	if (value() == index) { setValue(0); }
	else if (value() > index) { setValue(value() - 1); }
	setRange(0, maxValue() - 1);
}

void MixerChannelModel::channelCreated(int index)
{
	setRange(0, maxValue() + 1);
}

} // namespace lmms
