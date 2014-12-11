/*
 * eqeffect.cpp - defination of EqEffect class.
 *
 * Copyright (c) 2014 David French <dave/dot/french3/at/googlemail/dot/com>
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

#include "eqeffect.h"
#include "embed.cpp"
#include "lmms_math.h"
#include "BasicFilters.h"
#include "interpolation.h"
#include "Engine.h"
#include "MainWindow.h"

//TODO
//re write to store data from each filter(models,name, storeage name ) in a class, stored in array
//then just loop ever array for each section



extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT eq_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"Eq",
	QT_TRANSLATE_NOOP( "pluginBrowser", "A native eq plugin" ),
	"Dave French <contact/dot/dave/dot/french3/at/googlemail/dot/com>",
	0x0100,
	Plugin::Effect,
	new PluginPixmapLoader( "logo" ),
	NULL,
	NULL
} ;




EqEffect::EqEffect(Model *parent, const Plugin::Descriptor::SubPluginFeatures::Key *key) :
	Effect( &eq_plugin_descriptor, parent, key ),
	m_eqControls( this )
{
	m_dFilterCount = 10;
	m_downsampleFilters = new EqLp12Filter[m_dFilterCount];
	for( int i = 0; i < m_dFilterCount; i++)
	{
		m_downsampleFilters[i].setFrequency(22000);
		m_downsampleFilters[i].setQ(0.85);
		m_downsampleFilters[i].setGain(0);
		m_downsampleFilters[i].setSampleRate(Engine::mixer()->processingSampleRate() * 2 );
	}
	m_upBuf = 0;
}




EqEffect::~EqEffect()
{

}




bool EqEffect::processAudioBuffer(sampleFrame *buf, const fpp_t frames)
{
	if( !isEnabled() || !isRunning () )
	{
		return( false );
	}
	double outSum = 0.0;
	const float d = dryLevel();
	const float w = wetLevel();
	const float outGain = m_eqControls.m_outGainModel.value();
	const int sampleRate = Engine::mixer()->processingSampleRate() * 2;
	sample_t dryS[2];
	sampleFrame m_inPeak;

	analyze( buf, frames, &m_eqControls.m_inFftBands) ;
	//TODO UPSAMPLE
	upsample( buf, frames );

	gain(m_upBuf , m_upBufFrames, m_eqControls.m_inGainModel.value(), &m_inPeak );
	m_eqControls.m_inPeakL = m_eqControls.m_inPeakL < m_inPeak[0] ? m_inPeak[0] : m_eqControls.m_inPeakL;
	m_eqControls.m_inPeakR = m_eqControls.m_inPeakR < m_inPeak[1] ? m_inPeak[0] : m_eqControls.m_inPeakR;

	if(m_eqControls.m_hpActiveModel.value() ){
		m_hp12.setSampleRate( sampleRate );
		m_hp12.setFrequency( m_eqControls.m_hpFeqModel.value() );
		m_hp12.setQ( m_eqControls.m_hpResModel.value() );
		m_hp12.processBuffer( m_upBuf , m_upBufFrames );

		if( m_eqControls.m_hp24Model.value() || m_eqControls.m_hp48Model.value() )
		{
			m_hp24.setSampleRate( sampleRate );
			m_hp24.setFrequency( m_eqControls.m_hpFeqModel.value() );
			m_hp24.setQ( m_eqControls.m_hpResModel.value() );
			m_hp24.processBuffer( m_upBuf , m_upBufFrames );
		}

		if( m_eqControls.m_hp48Model.value() )
		{
			m_hp480.setSampleRate( sampleRate );
			m_hp480.setFrequency( m_eqControls.m_hpFeqModel.value() );
			m_hp480.setQ( m_eqControls.m_hpResModel.value() );
			m_hp480.processBuffer( m_upBuf , m_upBufFrames );

			m_hp481.setSampleRate( sampleRate );
			m_hp481.setFrequency( m_eqControls.m_hpFeqModel.value() );
			m_hp481.setQ( m_eqControls.m_hpResModel.value() );
			m_hp481.processBuffer( m_upBuf , m_upBufFrames );
		}
	}

	if( m_eqControls.m_lowShelfActiveModel.value() )
	{
		m_lowShelf.setSampleRate( sampleRate );
		m_lowShelf.setFrequency( m_eqControls.m_lowShelfFreqModel.value() );
		m_lowShelf.setQ( m_eqControls.m_lowShelfResModel .value() );
		m_lowShelf.setGain( m_eqControls.m_lowShelfGainModel.value() );
		m_lowShelf.processBuffer( m_upBuf , m_upBufFrames );
	}

	if( m_eqControls.m_para1ActiveModel.value() )
	{
		m_para1.setSampleRate(sampleRate );
		m_para1.setFrequency( m_eqControls.m_para1FreqModel.value() );
		m_para1.setQ( m_eqControls.m_para1ResModel.value() );
		m_para1.setGain( m_eqControls.m_para1GainModel.value() );
		m_para1.processBuffer( m_upBuf , m_upBufFrames );
	}

	if( m_eqControls.m_para2ActiveModel.value() )
	{
		m_para2.setSampleRate( sampleRate );
		m_para2.setFrequency( m_eqControls.m_para2FreqModel.value() );
		m_para2.setQ( m_eqControls.m_para2ResModel.value() );
		m_para2.setGain( m_eqControls.m_para2GainModel.value() );
		m_para2.processBuffer( m_upBuf , m_upBufFrames );
	}

	if( m_eqControls.m_para3ActiveModel.value() )
	{
		m_para3.setSampleRate( sampleRate);
		m_para3.setFrequency( m_eqControls.m_para3FreqModel.value() );
		m_para3.setQ( m_eqControls.m_para3ResModel.value() );
		m_para3.setGain( m_eqControls.m_para3GainModel.value() );
		m_para3.processBuffer( m_upBuf , m_upBufFrames );
	}

	if( m_eqControls.m_para4ActiveModel.value() )
	{
		m_para4.setSampleRate( sampleRate );
		m_para4.setFrequency( m_eqControls.m_para4FreqModel.value() );
		m_para4.setQ( m_eqControls.m_para4ResModel.value() );
		m_para4.setGain( m_eqControls.m_para4GainModel.value() );
		m_para4.processBuffer( m_upBuf , m_upBufFrames );
	}

	if( m_eqControls.m_highShelfActiveModel.value() )
	{
		m_highShelf.setSampleRate( sampleRate );
		m_highShelf.setFrequency( m_eqControls.m_highShelfFreqModel.value() );
		m_highShelf.setQ( m_eqControls.m_highShelfResModel.value() );
		m_highShelf.setGain( m_eqControls.m_highShelfGainModel.value() );
		m_highShelf.processBuffer( m_upBuf , m_upBufFrames );
	}

	if(m_eqControls.m_lpActiveModel.value() ){
		m_lp12.setSampleRate( sampleRate );
		m_lp12.setFrequency( m_eqControls.m_lpFreqModel.value() );
		m_lp12.setQ( m_eqControls.m_lpResModel.value() );
		m_lp12.processBuffer( m_upBuf , m_upBufFrames );

		if( m_eqControls.m_lp24Model.value() || m_eqControls.m_lp48Model.value() )
		{
			m_lp24.setSampleRate( sampleRate );
			m_lp24.setFrequency( m_eqControls.m_lpFreqModel.value() );
			m_lp24.setQ( m_eqControls.m_lpResModel.value() );
			m_lp24.processBuffer( m_upBuf , m_upBufFrames );
		}

		if( m_eqControls.m_lp48Model.value() )
		{
			m_lp480.setSampleRate( sampleRate );
			m_lp480.setFrequency( m_eqControls.m_lpFreqModel.value() );
			m_lp480.setQ( m_eqControls.m_lpResModel.value() );
			m_lp480.processBuffer( m_upBuf , m_upBufFrames );

			m_lp481.setSampleRate( sampleRate );
			m_lp481.setFrequency( m_eqControls.m_lpFreqModel.value() );
			m_lp481.setQ( m_eqControls.m_lpResModel.value() );
			m_lp481.processBuffer( m_upBuf , m_upBufFrames );
		}
	}

	sampleFrame outPeak;
	gain( m_upBuf , m_upBufFrames, outGain, &outPeak );
	m_eqControls.m_outPeakL = m_eqControls.m_outPeakL < outPeak[0] ? outPeak[0] : m_eqControls.m_outPeakL;
	m_eqControls.m_outPeakR = m_eqControls.m_outPeakR < outPeak[1] ? outPeak[0] : m_eqControls.m_outPeakR;

	//TODO lp filter before downsample
	for( int i =0; i < m_dFilterCount; i++)
	{
		m_downsampleFilters[i].processBuffer(m_upBuf , m_upBufFrames );
	}

	downSample( buf, frames );

	for( fpp_t f = 0; f < frames; ++f )
	{
		buf[f][0] = ( d * dryS[0] ) + ( w * buf[f][0] );
		buf[f][1] = ( d * dryS[1] ) + ( w * buf[f][1] );
		outSum += buf[f][0]*buf[f][0] + buf[f][1]*buf[f][1];
	}
	checkGate( outSum / frames );
	analyze( buf, frames, &m_eqControls.m_outFftBands) ;
	setBandPeaks( &m_eqControls.m_outFftBands , (int)(sampleRate * 0.5));

	return isRunning();
}




void EqEffect::gain(sampleFrame *buf, const fpp_t frames, float scale, sampleFrame* peak)
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

sampleFrame m_lastUpFrame;
void EqEffect::upsample(sampleFrame *buf, const fpp_t frames)
{

	if(m_upBufFrames != frames * 2 )
	{
		if( m_upBuf )
		{
			delete m_upBuf;
		}
		m_upBuf = new sampleFrame[frames * 2];
		m_upBufFrames = frames * 2;
	}

	for( int f = 0, f2 = 0; f < frames; ++f, f2 += 2 )
	{
		m_upBuf[f2][0] = linearInterpolate(m_lastUpFrame[0],buf[f][0], 0.5);
		m_upBuf[f2][1] = linearInterpolate(m_lastUpFrame[1],buf[f][1], 0.5);
		m_upBuf[f2+1][0] = buf[f][0];
		m_upBuf[f2+1][1] = buf[f][1];

		m_lastUpFrame[0] = buf[f][0];
		m_lastUpFrame[1] = buf[f][1];
	}
}

void EqEffect::downSample(sampleFrame *buf, const fpp_t frames)
{
	for( int f = 0, f2 = 0; f < frames; ++f, f2 += 2 )
	{
		buf[f][0] = m_upBuf[f2][0];
		buf[f][1] = m_upBuf[f2][1];
	}
}

void EqEffect::analyze(sampleFrame *buf, const fpp_t frames, FftBands* fft)
{
	const int FFT_BUFFER_SIZE = 2048;
	fpp_t f = 0;
	if( frames > FFT_BUFFER_SIZE )
	{
		fft->m_framesFilledUp = 0;
		f = frames - FFT_BUFFER_SIZE;
	}
	// meger channels
	for( ; f < frames; ++f )
	{
		fft->m_buffer[fft->m_framesFilledUp] =
			( buf[f][0] + buf[f][1] ) * 0.5;
		++fft->m_framesFilledUp;
	}

	if( fft->m_framesFilledUp < FFT_BUFFER_SIZE )
	{
		return;
	}

	fft->m_sr = Engine::mixer()->processingSampleRate();
	const int LOWEST_FREQ = 0;
	const int HIGHEST_FREQ = fft->m_sr / 2;

	fftwf_execute( fft->m_fftPlan );
	absspec( fft->m_specBuf, fft->m_absSpecBuf, FFT_BUFFER_SIZE+1 );

	compressbands( fft->m_absSpecBuf, fft->m_bands, FFT_BUFFER_SIZE+1,
				   MAX_BANDS,
				   (int)(LOWEST_FREQ*(FFT_BUFFER_SIZE+1)/(float)(fft->m_sr /2)),
				   (int)(HIGHEST_FREQ*(FFT_BUFFER_SIZE+1)/(float)(fft->m_sr /2)));
			   fft->m_energy = maximum( fft->m_bands, MAX_BANDS ) / maximum( fft->m_buffer, FFT_BUFFER_SIZE );
			   fft->m_framesFilledUp = 0;
}

float EqEffect::peakBand(float minF, float maxF, FftBands *fft, int sr)
{
	float peak = -60;
	float * b = fft->m_bands;
	float h = 0;
	for(int x = 0; x < MAX_BANDS; x++, b++)
	{
		if( bandToFreq( x ,sr)  >= minF && bandToFreq( x,sr ) <= maxF )
		{
			h = 20*( log10( *b / fft->m_energy ) );
			peak = h > peak ? h : peak;
		}
	}
	return (peak+100)/100;
}

void EqEffect::setBandPeaks(FftBands *fft, int samplerate )
{
	m_eqControls.m_lowShelfPeakR = m_eqControls.m_lowShelfPeakL =
			peakBand( 0,
					  m_eqControls.m_lowShelfFreqModel.value(), fft , samplerate );

	m_eqControls.m_para1PeakL = m_eqControls.m_para1PeakR =
			peakBand( m_eqControls.m_para1FreqModel.value()
					  - (m_eqControls.m_para1FreqModel.value() / m_eqControls.m_para1ResModel.value() * 0.5),
					  m_eqControls.m_para1FreqModel.value()
					  + (m_eqControls.m_para1FreqModel.value() / m_eqControls.m_para1ResModel.value() * 0.5), fft , samplerate );

	m_eqControls.m_para2PeakL = m_eqControls.m_para2PeakR =
			peakBand( m_eqControls.m_para2FreqModel.value()
					  - (m_eqControls.m_para2FreqModel.value() / m_eqControls.m_para2ResModel.value() * 0.5),
					  m_eqControls.m_para2FreqModel.value()
					  + (m_eqControls.m_para2FreqModel.value() / m_eqControls.m_para2ResModel.value() * 0.5), fft , samplerate );

	m_eqControls.m_para3PeakL = m_eqControls.m_para3PeakR =
			peakBand( m_eqControls.m_para3FreqModel.value()
					  - (m_eqControls.m_para3FreqModel.value() / m_eqControls.m_para3ResModel.value() * 0.5),
					  m_eqControls.m_para3FreqModel.value()
					  + (m_eqControls.m_para3FreqModel.value() / m_eqControls.m_para3ResModel.value() * 0.5), fft , samplerate );

	m_eqControls.m_para4PeakL = m_eqControls.m_para4PeakR =
			peakBand( m_eqControls.m_para4FreqModel.value()
					  - (m_eqControls.m_para4FreqModel.value() / m_eqControls.m_para4ResModel.value() * 0.5),
					  m_eqControls.m_para4FreqModel.value()
					  + (m_eqControls.m_para4FreqModel.value() / m_eqControls.m_para4ResModel.value() * 0.5), fft , samplerate );

	m_eqControls.m_highShelfPeakL = m_eqControls.m_highShelfPeakR =
			peakBand( m_eqControls.m_highShelfFreqModel.value(),
					  samplerate * 0.5 , fft, samplerate );
}





extern "C"
{

//needed for getting plugin out of shared lib
Plugin * PLUGIN_EXPORT lmms_plugin_main( Model* parent, void* data )
{
	return new EqEffect( parent , static_cast<const Plugin::Descriptor::SubPluginFeatures::Key *>( data ) );
}

}}
