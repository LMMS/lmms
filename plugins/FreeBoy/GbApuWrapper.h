/*
 * GbApuWrapper.h - Gb_Apu subclass which allows direct buffer access
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

#ifndef LMMS_GB_APU_WRAPPER_H
#define LMMS_GB_APU_WRAPPER_H

#include <Gb_Apu.h>
#include <Multi_Buffer.h>

namespace lmms
{


class GbApuWrapper : private Gb_Apu
{
public:
	GbApuWrapper() = default;
	~GbApuWrapper() = default;

	blargg_err_t setSampleRate(long sampleRate, long clockRate);
	void writeRegister(unsigned addr, int data) { Gb_Apu::write_register(fakeClock(), addr, data); }
	long samplesAvail() const;
	long readSamples(blip_sample_t* out, long count);
	void trebleEq(const blip_eq_t& eq) { Gb_Apu::treble_eq(eq); }
	void bassFreq(int freq);
	void endFrame(blip_time_t endTime);

private:
	Stereo_Buffer m_buf;

	// Fake CPU timing
	blip_time_t fakeClock() { return m_time += 4; }
	blip_time_t m_time = 0;
};


} // namespace lmms

#endif // LMMS_GB_APU_WRAPPER_H
