/*
 * spectrum_analyzer.h - spectrum anaylzer
 *
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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
#include "spectrumanalyzer_controls.h"


const int MAX_BANDS = 249;


class spectrumAnalyzer : public Effect
{
public:
	enum ChannelModes
	{
		MergeChannels,
		LeftChannel,
		RightChannel
	} ;

	spectrumAnalyzer( Model * _parent,
			const Descriptor::SubPluginFeatures::Key * _key );
	virtual ~spectrumAnalyzer();
	virtual bool processAudioBuffer( sampleFrame * _buf,
							const fpp_t _frames );

	virtual EffectControls * controls()
	{
		return( &m_saControls );
	}


private:
	spectrumAnalyzerControls m_saControls;

	fftwf_plan m_fftPlan;

	fftwf_complex * m_specBuf;
	float m_absSpecBuf[FFT_BUFFER_SIZE+1];
	float m_buffer[FFT_BUFFER_SIZE*2];
	int m_framesFilledUp;

	float m_bands[MAX_BANDS];
	float m_energy;

	friend class spectrumAnalyzerControls;
	friend class spectrumView;

} ;





#endif
