/*
 * Analyzer.cpp - definition of Analyzer class.
 *
 * Copyright (c) 2019 Martin Pavelek <he29/dot/HS/at/gmail/dot/com>
 *
 * Based partially on Eq plugin code,
 * Copyright (c) 2014-2017, David French <dave/dot/french3/at/googlemail/dot/com>
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

#include "Analyzer.h"

#include "embed.h"
#include "plugin_export.h"


extern "C" {
	Plugin::Descriptor PLUGIN_EXPORT analyzer_plugin_descriptor =
	{
		"spectrumanalyzer",
		"Spectrum Analyzer",
		QT_TRANSLATE_NOOP("pluginBrowser", "A graphical spectrum analyzer."),
		"Martin Pavelek <he29/dot/HS/at/gmail/dot/com>",
		0x0100,
		Plugin::Effect,
		new PluginPixmapLoader("logo"),
		NULL,
		NULL
	};
}


Analyzer::Analyzer(Model *parent, const Plugin::Descriptor::SubPluginFeatures::Key *key) :
	Effect(&analyzer_plugin_descriptor, parent, key),
	m_processor(&m_controls),
	m_controls(this)
{
}


// Take audio data and pass them to the spectrum processor.
// Skip processing if the controls dialog isn't visible, it would only waste CPU cycles.
bool Analyzer::processAudioBuffer(sampleFrame *buffer, const fpp_t frame_count)
{
	if (!isEnabled() || !isRunning ()) {return false;}
	if (m_controls.isViewVisible()) {m_processor.analyse(buffer, frame_count);}
	return isRunning();
}


extern "C" {
	// needed for getting plugin out of shared lib
	PLUGIN_EXPORT Plugin *lmms_plugin_main(Model *parent, void *data)
	{
		return new Analyzer(parent, static_cast<const Plugin::Descriptor::SubPluginFeatures::Key *>(data));
	}
}

