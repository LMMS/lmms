/*
 * Vectorscope.cpp - definition of Vectorscope class.
 *
 * Copyright (c) 2019 Martin Pavelek <he29/dot/HS/at/gmail/dot/com>
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

#include "Vectorscope.h"

#include "embed.h"
#include "plugin_export.h"

namespace lmms
{


extern "C" {
	Plugin::Descriptor PLUGIN_EXPORT vectorscope_plugin_descriptor =
	{
		LMMS_STRINGIFY(PLUGIN_NAME),
		"Vectorscope",
		QT_TRANSLATE_NOOP("PluginBrowser", "A stereo field visualizer."),
		"Martin Pavelek <he29/dot/HS/at/gmail/dot/com>",
		0x0100,
		Plugin::Type::Effect,
		new PluginPixmapLoader("logo"),
		nullptr,
		nullptr,
		nullptr,
	};
}


Vectorscope::Vectorscope(Model *parent, const Plugin::Descriptor::SubPluginFeatures::Key *key) :
	Effect(&vectorscope_plugin_descriptor, parent, key),
	m_controls(this),
	// Buffer is sized to cover 4* the current maximum LMMS audio buffer size,
	// so that it has some reserve space in case GUI thresd is busy.
	m_inputBuffer(4 * m_maxBufferSize)
{
}


// Take audio data and store them for processing and display in the GUI thread.
Effect::ProcessStatus Vectorscope::processImpl(SampleFrame* buf, const fpp_t frames)
{
	// Skip processing if the controls dialog isn't visible, it would only waste CPU cycles.
	if (m_controls.isViewVisible())
	{
		// To avoid processing spikes on audio thread, data are stored in
		// a lockless ringbuffer and processed in a separate thread.
		m_inputBuffer.write(buf, frames);
	}

	return ProcessStatus::Continue;
}


extern "C" {
	// needed for getting plugin out of shared lib
	PLUGIN_EXPORT Plugin *lmms_plugin_main(Model *parent, void *data)
	{
		return new Vectorscope(parent, static_cast<const Plugin::Descriptor::SubPluginFeatures::Key *>(data));
	}
}


} // namespace lmms
