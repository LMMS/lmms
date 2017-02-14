/*
 * eqeffect.cpp - defination of EqEffect class.
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

#include "EqEffect.h"

#include "embed.cpp"
#include "Engine.h"
#include "EqFader.h"
#include "interpolation.h"
#include "lmms_math.h"
#include "MainWindow.h"


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT eq_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"Equalizer",
	QT_TRANSLATE_NOOP( "pluginBrowser", "A native eq plugin" ),
	"Dave French <contact/dot/dave/dot/french3/at/googlemail/dot/com>",
	0x0100,
	Plugin::Effect,
	new PluginPixmapLoader( "logo" ),
	NULL,
	NULL
} ;

}


EqEffect::EqEffect( Model *parent, const Plugin::Descriptor::SubPluginFeatures::Key *key) :
	Effect( &eq_plugin_descriptor, parent, key ),
	m_eqControls( this ),
	m_inGain( 1.0 ),
	m_outGain( 1.0 )
{
}




EqEffect::~EqEffect()
{
}




bool EqEffect::processAudioBuffer( sampleFrame *buf, const fpp_t frames )
{
	// setup sample exact controls
	float hpRes = m_eqControls.m_hpResModel.value();
	float lowShelfRes = m_eqControls.m_lowShelfResModel.value();
	float para1Bw = m_eqControls.m_para1BwModel.value();
	float para2Bw = m_eqControls.m_para2BwModel.value();
	float para3Bw = m_eqControls.m_para3BwModel.value();
	float para4Bw = m_eqControls.m_para4BwModel.value();
	float highShelfRes = m_eqControls.m_highShelfResModel.value();
	float lpRes = m_eqControls.m_lpResModel.value();

	float hpFreq = m_eqControls.m_hpFeqModel.value();
	float lowShelfFreq = m_eqControls.m_lowShelfFreqModel.value();
	float para1Freq = m_eqControls.m_para1FreqModel.value();
	float para2Freq = m_eqControls.m_para2FreqModel.value();
	float para3Freq = m_eqControls.m_para3FreqModel.value();
	float para4Freq = m_eqControls.m_para4FreqModel.value();
	float highShelfFreq = m_eqControls.m_highShelfFreqModel.value();
	float lpFreq = m_eqControls.m_lpFreqModel.value();

	ValueBuffer *hpResBuffer = m_eqControls.m_hpResModel.valueBuffer();
	ValueBuffer *lowShelfResBuffer = m_eqControls.m_lowShelfResModel.valueBuffer();
	ValueBuffer *para1BwBuffer = m_eqControls.m_para1BwModel.valueBuffer();
	ValueBuffer *para2BwBuffer = m_eqControls.m_para2BwModel.valueBuffer();
	ValueBuffer *para3BwBuffer = m_eqControls.m_para3BwModel.valueBuffer();
	ValueBuffer *para4BwBuffer = m_eqControls.m_para4BwModel.valueBuffer();
	ValueBuffer *highShelfResBuffer = m_eqControls.m_highShelfResModel.valueBuffer();
	ValueBuffer *lpResBuffer = m_eqControls.m_lpResModel.valueBuffer();

	ValueBuffer *hpFreqBuffer = m_eqControls.m_hpFeqModel.valueBuffer();
	ValueBuffer *lowShelfFreqBuffer = m_eqControls.m_lowShelfFreqModel.valueBuffer();
	ValueBuffer *para1FreqBuffer = m_eqControls.m_para1FreqModel.valueBuffer();
	ValueBuffer *para2FreqBuffer = m_eqControls.m_para2FreqModel.valueBuffer();
	ValueBuffer *para3FreqBuffer = m_eqControls.m_para3FreqModel.valueBuffer();
	ValueBuffer *para4FreqBuffer = m_eqControls.m_para4FreqModel.valueBuffer();
	ValueBuffer *highShelfFreqBuffer = m_eqControls.m_highShelfFreqModel.valueBuffer();
	ValueBuffer *lpFreqBuffer = m_eqControls.m_lpFreqModel.valueBuffer();

	int hpResInc = hpResBuffer ? 1 : 0;
	int lowShelfResInc = lowShelfResBuffer ? 1 : 0;
	int para1BwInc = para1BwBuffer ? 1 : 0;
	int para2BwInc = para2BwBuffer ? 1 : 0;
	int para3BwInc = para3BwBuffer ? 1 : 0;
	int para4BwInc = para4BwBuffer ? 1 : 0;
	int highShelfResInc = highShelfResBuffer ? 1 : 0;
	int lpResInc = lpResBuffer ? 1 : 0;

	int hpFreqInc = hpFreqBuffer ? 1 : 0;
	int lowShelfFreqInc = lowShelfFreqBuffer ? 1 : 0;
	int para1FreqInc = para1FreqBuffer ? 1 : 0;
	int para2FreqInc = para2FreqBuffer ? 1 : 0;
	int para3FreqInc = para3FreqBuffer ? 1 : 0;
	int para4FreqInc = para4FreqBuffer ? 1 : 0;
	int highShelfFreqInc = highShelfFreqBuffer ? 1 : 0;
	int lpFreqInc = lpFreqBuffer ? 1 : 0;

	float *hpResPtr = hpResBuffer ? &( hpResBuffer->values()[ 0 ] ) : &hpRes;
	float *lowShelfResPtr = lowShelfResBuffer ? &( lowShelfResBuffer->values()[ 0 ] ) : &lowShelfRes;
	float *para1BwPtr = para1BwBuffer ? &( para1BwBuffer->values()[ 0 ] ) : &para1Bw;
	float *para2BwPtr = para2BwBuffer ? &( para2BwBuffer->values()[ 0 ] ) : &para2Bw;
	float *para3BwPtr = para3BwBuffer ? &( para3BwBuffer->values()[ 0 ] ) : &para3Bw;
	float *para4BwPtr = para4BwBuffer ? &( para4BwBuffer->values()[ 0 ] ) : &para4Bw;
	float *highShelfResPtr = highShelfResBuffer ? &( highShelfResBuffer->values()[ 0 ] ) : &highShelfRes;
	float *lpResPtr = lpResBuffer ? &( lpResBuffer->values()[ 0 ] ) : &lpRes;

	float *hpFreqPtr = hpFreqBuffer ? &( hpFreqBuffer->values()[ 0 ] ) : &hpFreq;
	float *lowShelfFreqPtr = lowShelfFreqBuffer ? &( lowShelfFreqBuffer->values()[ 0 ] ) : &lowShelfFreq;
	float *para1FreqPtr = para1FreqBuffer ? &(para1FreqBuffer->values()[ 0 ] ) : &para1Freq;
	float *para2FreqPtr = para2FreqBuffer ? &(para2FreqBuffer->values()[ 0 ] ) : &para2Freq;
	float *para3FreqPtr = para3FreqBuffer ? &(para3FreqBuffer->values()[ 0 ] ) : &para3Freq;
	float *para4FreqPtr = para4FreqBuffer ? &(para4FreqBuffer->values()[ 0 ] ) : &para4Freq;
	float *hightShelfFreqPtr = highShelfFreqBuffer ? &(highShelfFreqBuffer->values()[ 0 ] ) : &highShelfFreq;
	float *lpFreqPtr = lpFreqBuffer ? &(lpFreqBuffer ->values()[ 0 ] ) : &lpFreq;

	bool hpActive = m_eqControls.m_hpActiveModel.value();
	bool hp24Active = m_eqControls.m_hp24Model.value();
	bool hp48Active = m_eqControls.m_hp48Model.value();
	bool lowShelfActive = m_eqControls.m_lowShelfActiveModel.value();
	bool para1Active = m_eqControls.m_para1ActiveModel.value();
	bool para2Active = m_eqControls.m_para2ActiveModel.value();
	bool para3Active = m_eqControls.m_para3ActiveModel.value();
	bool para4Active = m_eqControls.m_para4ActiveModel.value();
	bool highShelfActive = m_eqControls.m_highShelfActiveModel.value();
	bool lpActive = m_eqControls.m_lpActiveModel.value();
	bool lp24Active = m_eqControls.m_lp24Model.value();
	bool lp48Active = m_eqControls.m_lp48Model.value();

	float lowShelfGain = m_eqControls.m_lowShelfGainModel.value();
	float para1Gain = m_eqControls.m_para1GainModel.value();
	float para2Gain = m_eqControls.m_para2GainModel.value();
	float para3Gain = m_eqControls.m_para3GainModel.value();
	float para4Gain = m_eqControls.m_para4GainModel.value();
	float highShelfGain = m_eqControls.m_highShelfGainModel.value();

	if( !isEnabled() || !isRunning () )
	{
		return( false );
	}

	if( m_eqControls.m_outGainModel.isValueChanged() )
	{
		m_outGain = dbfsToAmp(m_eqControls.m_outGainModel.value());
	}

	if( m_eqControls.m_inGainModel.isValueChanged() )
	{
		m_inGain = dbfsToAmp(m_eqControls.m_inGainModel.value());
	}

	m_eqControls.m_inProgress = true;
	double outSum = 0.0;

	for( fpp_t f = 0; f < frames; ++f )
	{
		outSum += buf[f][0]*buf[f][0] + buf[f][1]*buf[f][1];
	}

	const float outGain =  m_outGain;
	const int sampleRate = Engine::mixer()->processingSampleRate();
	sampleFrame m_inPeak = { 0, 0 };

	if(m_eqControls.m_analyseInModel.value( true ) &&  outSum > 0 )
	{
		m_eqControls.m_inFftBands.analyze( buf, frames );
	}
	else
	{
		m_eqControls.m_inFftBands.clear();
	}

	gain( buf, frames, m_inGain, &m_inPeak );
	m_eqControls.m_inPeakL = m_eqControls.m_inPeakL < m_inPeak[0] ? m_inPeak[0] : m_eqControls.m_inPeakL;
	m_eqControls.m_inPeakR = m_eqControls.m_inPeakR < m_inPeak[1] ? m_inPeak[1] : m_eqControls.m_inPeakR;

	for( fpp_t f = 0; f < frames; f++)
	{
		if( hpActive )
		{
			m_hp12.setParameters( sampleRate, *hpFreqPtr, *hpResPtr, 1 );
			buf[f][0] = m_hp12.update( buf[f][0], 0 );
			buf[f][1] = m_hp12.update( buf[f][1], 1 );

			if( hp24Active || hp48Active )
			{
				m_hp24.setParameters( sampleRate, *hpFreqPtr, *hpResPtr, 1 );
				buf[f][0] = m_hp24.update( buf[f][0], 0 );
				buf[f][1] = m_hp24.update( buf[f][1], 1 );
			}

			if( hp48Active )
			{
				m_hp480.setParameters( sampleRate, *hpFreqPtr, *hpResPtr, 1 );
				buf[f][0] = m_hp480.update( buf[f][0], 0 );
				buf[f][1] = m_hp480.update( buf[f][1], 1 );

				m_hp481.setParameters( sampleRate, *hpFreqPtr, *hpResPtr, 1 );
				buf[f][0] = m_hp481.update( buf[f][0], 0 );
				buf[f][1] = m_hp481.update( buf[f][1], 1 );
			}
		}

		if( lowShelfActive )
		{
			m_lowShelf.setParameters( sampleRate, *lowShelfFreqPtr, *lowShelfResPtr, lowShelfGain );
			buf[f][0] = m_lowShelf.update( buf[f][0], 0 );
			buf[f][1] = m_lowShelf.update( buf[f][1], 1 );
		}

		if( para1Active )
		{
			m_para1.setParameters( sampleRate, *para1FreqPtr, *para1BwPtr, para1Gain );
			buf[f][0] = m_para1.update( buf[f][0], 0 );
			buf[f][1] = m_para1.update( buf[f][1], 1 );
		}

		if( para2Active )
		{
			m_para2.setParameters( sampleRate, *para2FreqPtr, *para2BwPtr, para2Gain );
			buf[f][0] = m_para2.update( buf[f][0], 0 );
			buf[f][1] = m_para2.update( buf[f][1], 1 );
		}

		if( para3Active )
		{
			m_para3.setParameters( sampleRate, *para3FreqPtr, *para3BwPtr, para3Gain );
			buf[f][0] = m_para3.update( buf[f][0], 0 );
			buf[f][1] = m_para3.update( buf[f][1], 1 );
		}

		if( para4Active )
		{
			m_para4.setParameters( sampleRate, *para4FreqPtr, *para4BwPtr, para4Gain );
			buf[f][0] = m_para4.update( buf[f][0], 0 );
			buf[f][1] = m_para4.update( buf[f][1], 1 );
		}

		if( highShelfActive )
		{
			m_highShelf.setParameters( sampleRate, *hightShelfFreqPtr, *highShelfResPtr, highShelfGain );
			buf[f][0] = m_highShelf.update( buf[f][0], 0 );
			buf[f][1] = m_highShelf.update( buf[f][1], 1 );
		}

		if( lpActive ){
			m_lp12.setParameters( sampleRate, *lpFreqPtr, *lpResPtr, 1 );
			buf[f][0] = m_lp12.update( buf[f][0], 0 );
			buf[f][1] = m_lp12.update( buf[f][1], 1 );

			if( lp24Active || lp48Active )
			{
				m_lp24.setParameters( sampleRate, *lpFreqPtr, *lpResPtr, 1 );
				buf[f][0] = m_lp24.update( buf[f][0], 0 );
				buf[f][1] = m_lp24.update( buf[f][1], 1 );
			}

			if( lp48Active )
			{
				m_lp480.setParameters( sampleRate, *lpFreqPtr, *lpResPtr, 1 );
				buf[f][0] = m_lp480.update( buf[f][0], 0 );
				buf[f][1] = m_lp480.update( buf[f][1], 1 );

				m_lp481.setParameters( sampleRate, *lpFreqPtr, *lpResPtr, 1 );
				buf[f][0] = m_lp481.update( buf[f][0], 0 );
				buf[f][1] = m_lp481.update( buf[f][1], 1 );
			}
		}

		//increment pointers if needed
		hpResPtr += hpResInc;
		lowShelfResPtr += lowShelfResInc;
		para1BwPtr += para1BwInc;
		para2BwPtr += para2BwInc;
		para3BwPtr += para3BwInc;
		para4BwPtr += para4BwInc;
		highShelfResPtr += highShelfResInc;
		lpResPtr += lpResInc;

		hpFreqPtr += hpFreqInc;
		lowShelfFreqPtr += lowShelfFreqInc;
		para1FreqPtr += para1FreqInc;
		para2FreqPtr += para2FreqInc;
		para3FreqPtr += para3FreqInc;
		para4FreqPtr += para4FreqInc;
		hightShelfFreqPtr += highShelfFreqInc;
		lpFreqPtr += lpFreqInc;
	}

	sampleFrame outPeak = { 0, 0 };
	gain( buf, frames, outGain, &outPeak );
	m_eqControls.m_outPeakL = m_eqControls.m_outPeakL < outPeak[0] ? outPeak[0] : m_eqControls.m_outPeakL;
	m_eqControls.m_outPeakR = m_eqControls.m_outPeakR < outPeak[1] ? outPeak[1] : m_eqControls.m_outPeakR;

	checkGate( outSum / frames );

	if(m_eqControls.m_analyseOutModel.value( true ) && outSum > 0 )
	{
		m_eqControls.m_outFftBands.analyze( buf, frames );
		setBandPeaks( &m_eqControls.m_outFftBands , ( int )( sampleRate ) );
	}
	else
	{
		m_eqControls.m_outFftBands.clear();
	}

	m_eqControls.m_inProgress = false;
	return isRunning();
}




float EqEffect::peakBand( float minF, float maxF, EqAnalyser *fft, int sr )
{
	float peak = -60;
	float *b = fft->m_bands;
	float h = 0;
	for( int x = 0; x < MAX_BANDS; x++, b++ )
	{
		if( bandToFreq( x ,sr) >= minF && bandToFreq( x,sr ) <= maxF )
		{
			h = 20 * ( log10( *b / fft->getEnergy() ) );
			peak = h > peak ? h : peak;
		}
	}

	return ( peak + 60 ) / 100;
}




void EqEffect::setBandPeaks( EqAnalyser *fft, int samplerate )
{
	m_eqControls.m_lowShelfPeakR = m_eqControls.m_lowShelfPeakL =
			peakBand( m_eqControls.m_lowShelfFreqModel.value()
					  * ( 1 - m_eqControls.m_lowShelfResModel.value() * 0.5 ),
					  m_eqControls.m_lowShelfFreqModel.value(),
					  fft , samplerate );

	m_eqControls.m_para1PeakL = m_eqControls.m_para1PeakR =
			peakBand( m_eqControls.m_para1FreqModel.value()
					  * ( 1 - m_eqControls.m_para1BwModel.value() * 0.5 ),
					  m_eqControls.m_para1FreqModel.value()
					  * ( 1 + m_eqControls.m_para1BwModel.value() * 0.5 ),
					  fft , samplerate );

	m_eqControls.m_para2PeakL = m_eqControls.m_para2PeakR =
			peakBand( m_eqControls.m_para2FreqModel.value()
					  * ( 1 - m_eqControls.m_para2BwModel.value() * 0.5 ),
					  m_eqControls.m_para2FreqModel.value()
					  * ( 1 + m_eqControls.m_para2BwModel.value() * 0.5 ),
					  fft , samplerate );

	m_eqControls.m_para3PeakL = m_eqControls.m_para3PeakR =
			peakBand( m_eqControls.m_para3FreqModel.value()
					  * ( 1 - m_eqControls.m_para3BwModel.value() * 0.5 ),
					  m_eqControls.m_para3FreqModel.value()
					  * ( 1 + m_eqControls.m_para3BwModel.value() * 0.5 ),
					  fft , samplerate );

	m_eqControls.m_para4PeakL = m_eqControls.m_para4PeakR =
			peakBand( m_eqControls.m_para4FreqModel.value()
					  * ( 1 - m_eqControls.m_para4BwModel.value() * 0.5 ),
					  m_eqControls.m_para4FreqModel.value()
					  * ( 1 + m_eqControls.m_para4BwModel.value() * 0.5 ),
					  fft , samplerate );

	m_eqControls.m_highShelfPeakL = m_eqControls.m_highShelfPeakR =
			peakBand( m_eqControls.m_highShelfFreqModel.value(),
					  m_eqControls.m_highShelfFreqModel.value()
					  * ( 1 + m_eqControls.m_highShelfResModel.value() * 0.5 ),
					  fft, samplerate );
}

extern "C"
{

//needed for getting plugin out of shared lib
Plugin * PLUGIN_EXPORT lmms_plugin_main( Model* parent, void* data )
{
	return new EqEffect( parent , static_cast<const Plugin::Descriptor::SubPluginFeatures::Key *>( data ) );
}

}
