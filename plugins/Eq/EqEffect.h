/* eqeffect.h - defination of EqEffect class.
*
* Copyright (c) 2014 David French <dave/dot/french3/at/googlemail/dot/com>
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

#ifndef EQEFFECT_H
#define EQEFFECT_H

#include "BasicFilters.h"
#include "Effect.h"
#include "EqControls.h"
#include "EqFilter.h"
#include "lmms_math.h"



class EqEffect : public Effect
{
public:
	EqEffect( Model * parent , const Descriptor::SubPluginFeatures::Key * key );
	virtual ~EqEffect();
	virtual bool processAudioBuffer( sampleFrame * buf, const fpp_t frames );
	virtual EffectControls * controls()
	{
		return &m_eqControls;
	}
	inline void  gain( sampleFrame * buf, const fpp_t frames, float scale, sampleFrame * peak )
	{
		peak[0][0] = 0.0f; peak[0][1] = 0.0f;
		for( fpp_t f = 0; f < frames; ++f )
		{
			buf[f][0] *= scale;
			buf[f][1] *= scale;

			if( fabs( buf[f][0] ) > peak[0][0] )
			{
				peak[0][0] = fabs( buf[f][0] );
			}
			if( fabs( buf[f][1] ) > peak[0][1] )
			{
				peak[0][1] = fabs( buf[f][0] );
			}

		}
	}

private:
	EqControls m_eqControls;

	EqHp12Filter m_hp12;
	EqHp12Filter m_hp24;
	EqHp12Filter m_hp480;
	EqHp12Filter m_hp481;

	EqLowShelfFilter m_lowShelf;

	EqPeakFilter m_para1;
	EqPeakFilter m_para2;
	EqPeakFilter m_para3;
	EqPeakFilter m_para4;

	EqHighShelfFilter m_highShelf;

	EqLp12Filter m_lp12;
	EqLp12Filter m_lp24;
	EqLp12Filter m_lp480;
	EqLp12Filter m_lp481;

	float m_inGain;
	float m_outGain;

	float peakBand( float minF, float maxF, EqAnalyser *, int );

	inline float bandToFreq ( int index , int sampleRate )
	{
		return index * sampleRate / ( MAX_BANDS * 2 );
	}

	void setBandPeaks( EqAnalyser * fft , int );
};

#endif // EQEFFECT_H
