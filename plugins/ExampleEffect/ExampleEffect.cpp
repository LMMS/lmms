/*
 * ExampleEffect.cpp - Example effect boilerplate code
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "ExampleEffect.h"

#include "embed.h"
#include "plugin_export.h"

namespace lmms
{

extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT exampleeffect_plugin_descriptor =
{
	LMMS_STRINGIFY(PLUGIN_NAME),
	"Example Effect",
	QT_TRANSLATE_NOOP("PluginBrowser", "Example effect"),
	"Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>",
	0x0100,
	Plugin::Type::Effect,
	new PluginPixmapLoader("logo"),
	nullptr,
	nullptr,
} ;

}


ExampleEffectEffect::ExampleEffectEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key) :
	Effect(&exampleeffect_plugin_descriptor, parent, key),
	m_controls(this),
	m_smoothedValue(0.0f, 0.0f)
{
}


Effect::ProcessStatus ExampleEffectEffect::processImpl(SampleFrame* buffer, const fpp_t frames)
{
	// Here is where you put all of your audio processing code

	// In this demo, the effect performs a smoothing decaying interpolation based on the input frames, kind
	// of like a simple lowpass filter.

	// Setup variables before the loop
	float decayMutliplier = m_controls.m_decayModel.value();
	
	// Loop over all input samples in this buffer
	// f_cnt_t is an arbitrary data type for storing a frame count. You can also use ints if you want.
	for (f_cnt_t f = 0; f < frames; ++f)
	{
		SampleFrame currentFrame = buffer[f];
		// Now do the processing for this frame.
		// This demo effect simply interpolates m_smoothedValue with the input, and outputs the current smoothed value.
		m_smoothedValue = currentFrame * decayMutliplier + m_smoothedValue * (1 - decayMutliplier);
		// Update the value in the buffer
		buffer[f] = m_smoothedValue;
	}

	return ProcessStatus::ContinueIfNotQuiet;
}


extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin* lmms_plugin_main(Model* parent, void* data)
{
	return new ExampleEffectEffect(parent, static_cast<const Plugin::Descriptor::SubPluginFeatures::Key*>(data));
}

}

} // namespace lmms
