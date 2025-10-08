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

#ifdef SA_DEBUG
	#include <chrono>
	#include <iostream>
#endif

#include "embed.h"
#include "LmmsTypes.h"
#include "plugin_export.h"

namespace lmms
{


extern "C" {
	Plugin::Descriptor PLUGIN_EXPORT analyzer_plugin_descriptor =
	{
		"spectrumanalyzer",
		"Spectrum Analyzer",
		QT_TRANSLATE_NOOP("PluginBrowser", "A graphical spectrum analyzer."),
		"Martin Pavelek <he29/dot/HS/at/gmail/dot/com>",
		0x0112,
		Plugin::Type::Effect,
		new PluginPixmapLoader("logo"),
		{},
		nullptr,
	};
}


Analyzer::Analyzer(Model *parent, const Plugin::Descriptor::SubPluginFeatures::Key *key) :
	Effect(&analyzer_plugin_descriptor, parent, key),
	m_processor(&m_controls),
	m_controls(this),
	m_processorThread(m_processor, m_inputBuffer),
	// Buffer is sized to cover 4* the current maximum LMMS audio buffer size,
	// so that it has some reserve space in case data processor is busy.
	m_inputBuffer(4 * m_maxBufferSize)
{
	m_processorThread.start();
}


Analyzer::~Analyzer()
{
	m_processor.terminate();
	m_inputBuffer.wakeAll();
	m_processorThread.wait();
}

// Take audio data and pass them to the spectrum processor.
Effect::ProcessStatus Analyzer::processImpl(SampleFrame* buf, const fpp_t frames)
{
	// Measure time spent in audio thread; both average and peak should be well under 1 ms.
	#ifdef SA_DEBUG
		unsigned int audio_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
		if (audio_time - m_last_dump_time > 5000000000)	// print every 5 seconds
		{
			std::cout << "Analyzer audio thread: " << m_sum_execution / m_dump_count << " ms avg / "
				<< m_max_execution << " ms peak." << std::endl;
			m_last_dump_time = audio_time;
			m_sum_execution = m_max_execution = m_dump_count = 0;
		}
	#endif

	// Skip processing if the controls dialog isn't visible, it would only waste CPU cycles.
	if (m_controls.isViewVisible())
	{
		// To avoid processing spikes on audio thread, data are stored in
		// a lockless ringbuffer and processed in a separate thread.
		m_inputBuffer.write(buf, frames, true);
	}
	#ifdef SA_DEBUG
		audio_time = std::chrono::high_resolution_clock::now().time_since_epoch().count() - audio_time;
		m_dump_count++;
		m_sum_execution += audio_time / 1000000.0;
		if (audio_time / 1000000.0 > m_max_execution) {m_max_execution = audio_time / 1000000.0;}
	#endif

	return ProcessStatus::Continue;
}


extern "C" {
	// needed for getting plugin out of shared lib
	PLUGIN_EXPORT Plugin *lmms_plugin_main(Model *parent, void *data)
	{
		return new Analyzer(parent, static_cast<const Plugin::Descriptor::SubPluginFeatures::Key *>(data));
	}
}


} // namespace lmms
