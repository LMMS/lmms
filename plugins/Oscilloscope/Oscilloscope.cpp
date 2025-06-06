/*
 * Oscilloscope.cpp - Oscilloscope effect to preview the incoming waveform
 *
 * Copyright (c) 2025 Keratin
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

#include "Oscilloscope.h"

#include "embed.h"
#include "plugin_export.h"

namespace lmms
{

extern "C"
{
Plugin::Descriptor PLUGIN_EXPORT oscilloscope_plugin_descriptor =
{
	LMMS_STRINGIFY(PLUGIN_NAME),
	"Oscilloscope",
	QT_TRANSLATE_NOOP("PluginBrowser", "Oscilloscope plugin to display the incoming audio waveform"),
	"Keratin <3",
	0x0100,
	Plugin::Type::Effect,
	new PluginPixmapLoader("logo"),
	nullptr,
	nullptr,
};
}


Oscilloscope::Oscilloscope(Model* parent, const Descriptor::SubPluginFeatures::Key* key) :
	Effect(&oscilloscope_plugin_descriptor, parent, key),
	m_controls(this)
{
	m_ringBuffer = new SampleFrame[BUFFER_SIZE];
}

Oscilloscope::~Oscilloscope()
{
	delete[] m_ringBuffer;
}


Effect::ProcessStatus Oscilloscope::processImpl(SampleFrame* buffer, const fpp_t frames)
{
	if (!m_controls.m_pauseModel.value())
	{
		for (f_cnt_t f = 0; f < frames; ++f)
		{
			m_ringBuffer[m_ringBufferIndex] = buffer[f];
			m_ringBufferIndex = (m_ringBufferIndex + 1) % BUFFER_SIZE;
		}
	}
	return ProcessStatus::ContinueIfNotQuiet;
}


extern "C"
{
// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin* lmms_plugin_main(Model* parent, void* data)
{
	return new Oscilloscope(parent, static_cast<const Plugin::Descriptor::SubPluginFeatures::Key*>(data));
}
}

} // namespace lmms
