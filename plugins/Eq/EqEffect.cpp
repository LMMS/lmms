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

#include "EqEffect.h"
#include "embed.h"
#include "lmms_math.h"
#include "BasicFilters.h"
#include "interpolation.h"
#include "Engine.h"
#include "MainWindow.h"
#include "EqFader.h"

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


EqEffect::EqEffect(Model *parent, const Plugin::Descriptor::SubPluginFeatures::Key *key) :
	Effect( &eq_plugin_descriptor, parent, key ),
	m_eqControls( this ),
	m_inGain( 1.0 ),
	m_outGain( 1.0 )
{

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
	if( m_eqControls.m_outGainModel.isValueChanged() )
	{
		m_outGain = dbvToAmp(m_eqControls.m_outGainModel.value());
	}
	if( m_eqControls.m_inGainModel.isValueChanged() )
	{
		m_inGain = dbvToAmp(m_eqControls.m_inGainModel.value());
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

	if(m_eqControls.m_analyseIn )
	{
		m_eqControls.m_inFftBands.analyze( buf, frames );
	}
	else
	{
		m_eqControls.m_inFftBands.clear();
	}
	gain(buf , frames, m_inGain , &m_inPeak );
	m_eqControls.m_inPeakL = m_eqControls.m_inPeakL < m_inPeak[0] ? m_inPeak[0] : m_eqControls.m_inPeakL;
	m_eqControls.m_inPeakR = m_eqControls.m_inPeakR < m_inPeak[1] ? m_inPeak[1] : m_eqControls.m_inPeakR;

	if(m_eqControls.m_hpActiveModel.value() ){

		m_hp12.setParameters( sampleRate, m_eqControls.m_hpFeqModel.value(), m_eqControls.m_hpResModel.value(), 1 );
		m_hp12.processBuffer( buf, frames );

		if( m_eqControls.m_hp24Model.value() || m_eqControls.m_hp48Model.value() )
		{
			m_hp24.setParameters( sampleRate, m_eqControls.m_hpFeqModel.value(), m_eqControls.m_hpResModel.value(), 1 );
			m_hp24.processBuffer( buf, frames );
		}

		if( m_eqControls.m_hp48Model.value() )
		{
			m_hp480.setParameters( sampleRate, m_eqControls.m_hpFeqModel.value(), m_eqControls.m_hpResModel.value(), 1 );
			m_hp480.processBuffer( buf, frames );

			m_hp481.setParameters( sampleRate, m_eqControls.m_hpFeqModel.value(), m_eqControls.m_hpResModel.value(), 1 );
			m_hp481.processBuffer( buf, frames );
		}
	}

	if( m_eqControls.m_lowShelfActiveModel.value() )
	{
		m_lowShelf.setParameters( sampleRate, m_eqControls.m_lowShelfFreqModel.value(), m_eqControls.m_lowShelfResModel .value(), m_eqControls.m_lowShelfGainModel.value() );
		m_lowShelf.processBuffer( buf, frames );
	}

	if( m_eqControls.m_para1ActiveModel.value() )
	{
		m_para1.setParameters( sampleRate, m_eqControls.m_para1FreqModel.value(), m_eqControls.m_para1BwModel.value(), m_eqControls.m_para1GainModel.value() );
		m_para1.processBuffer( buf, frames );
	}

	if( m_eqControls.m_para2ActiveModel.value() )
	{
		m_para2.setParameters( sampleRate, m_eqControls.m_para2FreqModel.value(), m_eqControls.m_para2BwModel.value(), m_eqControls.m_para2GainModel.value() );
		m_para2.processBuffer( buf, frames );
	}

	if( m_eqControls.m_para3ActiveModel.value() )
	{
		m_para3.setParameters( sampleRate, m_eqControls.m_para3FreqModel.value(), m_eqControls.m_para3BwModel.value(), m_eqControls.m_para3GainModel.value() );
		m_para3.processBuffer( buf, frames );
	}

	if( m_eqControls.m_para4ActiveModel.value() )
	{
		m_para4.setParameters( sampleRate, m_eqControls.m_para4FreqModel.value(), m_eqControls.m_para4BwModel.value(), m_eqControls.m_para4GainModel.value() );
		m_para4.processBuffer( buf, frames );
	}

	if( m_eqControls.m_highShelfActiveModel.value() )
	{
		m_highShelf.setParameters( sampleRate, m_eqControls.m_highShelfFreqModel.value(), m_eqControls.m_highShelfResModel.value(), m_eqControls.m_highShelfGainModel.value());
		m_highShelf.processBuffer( buf, frames );
	}

	if(m_eqControls.m_lpActiveModel.value() ){
		m_lp12.setParameters( sampleRate, m_eqControls.m_lpFreqModel.value(), m_eqControls.m_lpResModel.value(), 1 );
		m_lp12.processBuffer( buf, frames );

		if( m_eqControls.m_lp24Model.value() || m_eqControls.m_lp48Model.value() )
		{
			m_lp24.setParameters( sampleRate, m_eqControls.m_lpFreqModel.value(), m_eqControls.m_lpResModel.value(), 1 );
			m_lp24.processBuffer( buf, frames );
		}

		if( m_eqControls.m_lp48Model.value() )
		{
			m_lp480.setParameters( sampleRate, m_eqControls.m_lpFreqModel.value(), m_eqControls.m_lpResModel.value(), 1 );
			m_lp480.processBuffer( buf, frames );

			m_lp481.setParameters( sampleRate, m_eqControls.m_lpFreqModel.value(), m_eqControls.m_lpResModel.value(), 1 );
			m_lp481.processBuffer( buf, frames );
		}
	}

	sampleFrame outPeak = { 0, 0 };
	gain( buf, frames, outGain, &outPeak );
	m_eqControls.m_outPeakL = m_eqControls.m_outPeakL < outPeak[0] ? outPeak[0] : m_eqControls.m_outPeakL;
	m_eqControls.m_outPeakR = m_eqControls.m_outPeakR < outPeak[1] ? outPeak[1] : m_eqControls.m_outPeakR;

	checkGate( outSum / frames );
	if(m_eqControls.m_analyseOut )
	{
		m_eqControls.m_outFftBands.analyze( buf, frames );
		setBandPeaks( &m_eqControls.m_outFftBands , ( int )( sampleRate * 0.5 ) );
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

void EqEffect::setBandPeaks(EqAnalyser *fft, int samplerate )
{
	m_eqControls.m_lowShelfPeakR = m_eqControls.m_lowShelfPeakL =
			peakBand( 0,
					  m_eqControls.m_lowShelfFreqModel.value(), fft , samplerate );

	m_eqControls.m_para1PeakL = m_eqControls.m_para1PeakR =
			peakBand( m_eqControls.m_para1FreqModel.value()
					  - (m_eqControls.m_para1FreqModel.value() * m_eqControls.m_para1BwModel.value() * 0.5),
					  m_eqControls.m_para1FreqModel.value()
					  + (m_eqControls.m_para1FreqModel.value() * m_eqControls.m_para1BwModel.value() * 0.5),
					  fft , samplerate );

	m_eqControls.m_para2PeakL = m_eqControls.m_para2PeakR =
			peakBand( m_eqControls.m_para2FreqModel.value()
					  - (m_eqControls.m_para2FreqModel.value() * m_eqControls.m_para2BwModel.value() * 0.5),
					  m_eqControls.m_para2FreqModel.value()
					  + (m_eqControls.m_para2FreqModel.value() * m_eqControls.m_para2BwModel.value() * 0.5),
					  fft , samplerate );

	m_eqControls.m_para3PeakL = m_eqControls.m_para3PeakR =
			peakBand( m_eqControls.m_para3FreqModel.value()
					  - (m_eqControls.m_para3FreqModel.value() * m_eqControls.m_para3BwModel.value() * 0.5),
					  m_eqControls.m_para3FreqModel.value()
					  + (m_eqControls.m_para3FreqModel.value() * m_eqControls.m_para3BwModel.value() * 0.5),
					  fft , samplerate );

	m_eqControls.m_para4PeakL = m_eqControls.m_para4PeakR =
			peakBand( m_eqControls.m_para4FreqModel.value()
					  - (m_eqControls.m_para4FreqModel.value() * m_eqControls.m_para4BwModel.value() * 0.5),
					  m_eqControls.m_para4FreqModel.value()
					  + (m_eqControls.m_para4FreqModel.value() * m_eqControls.m_para4BwModel.value() * 0.5),
					  fft , samplerate );

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

}
