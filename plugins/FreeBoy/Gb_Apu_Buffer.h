/*
 * Gb_Apu_Buffer.cpp - Gb_Apu subclass which allows direct buffer access
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
#ifndef GB_APU_BUFFER_H
#define GB_APU_BUFFER_H

#include "Gb_Apu.h"
#include "Multi_Buffer.h"
#include "MemoryManager.h"

class Gb_Apu_Buffer : public Gb_Apu {
	MM_OPERATORS
public:
	Gb_Apu_Buffer();
	~Gb_Apu_Buffer();

	void end_frame(blip_time_t);

	blargg_err_t set_sample_rate(long sample_rate, long clock_rate);
	long samples_avail() const;
	typedef blip_sample_t sample_t;
	long read_samples(sample_t* out, long count);
	void bass_freq(int freq);
private:
	Stereo_Buffer m_buf;
};

#endif

