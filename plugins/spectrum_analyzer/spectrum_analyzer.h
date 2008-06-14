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

#include <fftw3.h>

#include "effect.h"
#include "spectrumanalyzer_controls.h"


const int MAX_BANDS = 400;
const int BUFFER_SIZE = 2048;


class spectrumAnalyzer : public effect
{
public:
	enum ChannelModes
	{
		MergeChannels,
		LeftChannel,
		RightChannel
	} ;

	spectrumAnalyzer( model * _parent,
			const descriptor::subPluginFeatures::key * _key );
	virtual ~spectrumAnalyzer();
	virtual bool processAudioBuffer( sampleFrame * _buf,
							const fpp_t _frames );

	virtual effectControls * getControls( void )
	{
		return( &m_saControls );
	}


private:
	spectrumAnalyzerControls m_saControls;

	fftw_plan m_fftPlan;

	fftw_complex * m_specBuf;
	double m_absSpecBuf[BUFFER_SIZE+1];
	double m_buffer[BUFFER_SIZE*2];
	int m_framesFilledUp;

	double m_bands[MAX_BANDS];
	double m_energy;

	friend class spectrumAnalyzerControls;
	friend class spectrumView;

} ;





#endif
