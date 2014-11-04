/*
 * SpectrumAnalyzer.h - spectrum anaylzer effect plugin
 *
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of LMMS - http://lmms.io
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


#ifndef _SPECTRUM_ANALYZER_H
#define _SPECTRUM_ANALYZER_H

#include "Effect.h"
#include "fft_helpers.h"
#include "SpectrumAnalyzerControls.h"


const int MAX_BANDS = 249;


class SpectrumAnalyzer : public Effect
{
public:
	enum ChannelModes
	{
		MergeChannels,
		LeftChannel,
		RightChannel
	} ;

	SpectrumAnalyzer( Model * _parent,
			const Descriptor::SubPluginFeatures::Key * _key );
	virtual ~SpectrumAnalyzer();
	virtual bool processAudioBuffer( sampleFrame * _buf,
							const fpp_t _frames );

	virtual EffectControls * controls()
	{
		return( &m_saControls );
	}


private:
	SpectrumAnalyzerControls m_saControls;

	fftwf_plan m_fftPlan;

	fftwf_complex * m_specBuf;
	float m_absSpecBuf[FFT_BUFFER_SIZE+1];
	float m_buffer[FFT_BUFFER_SIZE*2];
	int m_framesFilledUp;

	float m_bands[MAX_BANDS];
	float m_energy;

	friend class SpectrumAnalyzerControls;
	friend class SpectrumView;

} ;


#endif
