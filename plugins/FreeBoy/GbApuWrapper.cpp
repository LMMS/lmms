/*
 * GbApuWrapper.cpp - Gb_Apu subclass which allows direct buffer access
 * Copyright (c) 2017 Tres Finocchiaro <tres.finocchiaro/at/gmail.com>
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

#include "GbApuWrapper.h"

namespace lmms
{


// Sets specified sample rate and clock rate in Stereo_Buffer
blargg_err_t GbApuWrapper::setSampleRate(long sampleRate, long clockRate)
{
	Gb_Apu::output(m_buf.center(), m_buf.left(), m_buf.right());
	m_buf.clock_rate(clockRate);
	return m_buf.set_sample_rate(sampleRate);
}

// Wrap Stereo_Buffer::samples_avail()
long GbApuWrapper::samplesAvail() const
{
	return m_buf.samples_avail();
}

// Wrap Stereo_Buffer::read_samples(...)
long GbApuWrapper::readSamples(blip_sample_t* out, long count)
{
	return m_buf.read_samples(out, count);
}

// Wrap Stereo_Buffer::bass_freq(...)
void GbApuWrapper::bassFreq(int freq)
{
	m_buf.bass_freq(freq);
}

void GbApuWrapper::endFrame(blip_time_t endTime)
{
	m_time = 0;
	Gb_Apu::end_frame(endTime);
	m_buf.end_frame(endTime);
}


} // namespace lmms
