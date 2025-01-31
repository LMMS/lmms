/*
 * AudioPluginConfig.h - Audio plugin configuration
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

#ifndef LMMS_AUDIO_PLUGIN_CONFIG_H
#define LMMS_AUDIO_PLUGIN_CONFIG_H

#include "AudioData.h"

namespace lmms {

//! Compile time customizations for an audio plugin
struct AudioPluginConfig
{
	//! The audio data type used by the plugin
	AudioDataKind kind;

	//! The audio data layout used by the plugin: interleaved or non-interleaved
	bool interleaved;

	//! The number of plugin input channels, or `DynamicChannelCount` if unknown at compile time
	int inputs = DynamicChannelCount;

	//! The number of plugin output channels, or `DynamicChannelCount` if unknown at compile time
	int outputs = DynamicChannelCount;

	//! In-place processing - true (always in-place) or false (dynamic, customizable in audio buffer impl)
	bool inplace = false;
};

} // namespace lmms

#endif // LMMS_AUDIO_PLUGIN_CONFIG_H
