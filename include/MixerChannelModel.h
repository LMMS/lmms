/*
 * MixerChannelModel.h
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

#ifndef LMMS_MIXER_CHANNEL_MODEL_H
#define LMMS_MIXER_CHANNEL_MODEL_H

#include "AutomatableModel.h"

namespace lmms {
/**
 * @brief An @ref IntModel that keeps track of its assigned mixer channel.
 * 
 * This model tracks its assigned mixer channel. It is a subclass of IntModel that adds functionality to handle channel
 * creation, deletion, and swapping. Both the value and valid range are automatically updated to reflect these changes.
 */
class MixerChannelModel : public IntModel
{
public:
	MixerChannelModel(Model* parent = nullptr);

private:
	void channelsSwapped(int fromIndex, int toIndex);
	void channelDeleted(int index);
	void channelCreated(int index);
};

} // namespace lmms

#endif // LMMS_MIXER_CHANNEL_MODEL_H