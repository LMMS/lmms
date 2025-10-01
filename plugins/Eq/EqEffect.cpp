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

#include "Engine.h"
#include "lmms_math.h"

#include "embed.h"
#include "plugin_export.h"

namespace lmms
{


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT eq_plugin_descriptor =
{
	LMMS_STRINGIFY( PLUGIN_NAME ),
	"Equalizer",
	QT_TRANSLATE_NOOP( "PluginBrowser", "A native eq plugin" ),
	"Dave French <contact/dot/dave/dot/french3/at/googlemail/dot/com>",
	0x0100,
	Plugin::Type::Effect,
	new PluginPixmapLoader("logo"),
	nullptr,
	nullptr,
	nullptr,
} ;

}


EqEffect::EqEffect( Model *parent, const Plugin::Descriptor::SubPluginFeatures::Key *key) :
	Effect( &eq_plugin_descriptor, parent, key ),
	m_eqControls( this ),
	m_inGain( 1.0 ),
	m_outGain( 1.0 )
{
}




Effect::ProcessStatus EqEffect::processImpl(SampleFrame* buf, const fpp_t frames)
{
	const int sampleRate = Engine::audioEngine()->outputSampleRate();

	//wet/dry controls
	const float dry = dryLevel();
	const float wet = wetLevel();
	auto dryS = std::array<sample_t, 2>{};
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

	//set all filter parameters once per frame, EqFilter handles
	//smooth xfading, reducing pops clicks and dc bias offsets

	m_hp12.setParameters( sampleRate, hpFreq, hpRes, 1 );
	m_hp24.setParameters( sampleRate, hpFreq, hpRes, 1 );
	m_hp480.setParameters( sampleRate, hpFreq, hpRes, 1 );
	m_hp481.setParameters( sampleRate, hpFreq, hpRes, 1 );
	m_lowShelf.setParameters( sampleRate, lowShelfFreq, lowShelfRes, lowShelfGain );
	m_para1.setParameters( sampleRate, para1Freq, para1Bw, para1Gain );
	m_para2.setParameters( sampleRate, para2Freq, para2Bw, para2Gain );
	m_para3.setParameters( sampleRate, para3Freq, para3Bw, para3Gain );
	m_para4.setParameters( sampleRate, para4Freq, para4Bw, para4Gain );
	m_highShelf.setParameters( sampleRate, highShelfFreq, highShelfRes, highShelfGain );
	m_lp12.setParameters( sampleRate, lpFreq, lpRes, 1 );
	m_lp24.setParameters( sampleRate, lpFreq, lpRes, 1 );
	m_lp480.setParameters( sampleRate, lpFreq, lpRes, 1 );
	m_lp481.setParameters( sampleRate, lpFreq, lpRes, 1 );


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

	for (fpp_t f = 0; f < frames; ++f)
	{
		outSum += buf[f][0] * buf[f][0] + buf[f][1] * buf[f][1];
	}

	const float outGain =  m_outGain;
	SampleFrame m_inPeak = { 0, 0 };

	if(m_eqControls.m_analyseInModel.value( true ) &&  outSum > 0 && m_eqControls.isViewVisible()  )
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

	float periodProgress = 0.0f; // percentage of period processed
	for( fpp_t f = 0; f < frames; ++f)
	{
		periodProgress = (float)f / (float)(frames-1);
		//wet dry buffer
		dryS[0] = buf[f][0];
		dryS[1] = buf[f][1];
		if( hpActive )
		{
			buf[f][0] = m_hp12.update( buf[f][0], 0, periodProgress );
			buf[f][1] = m_hp12.update( buf[f][1], 1, periodProgress );

			if( hp24Active || hp48Active )
			{
				buf[f][0] = m_hp24.update( buf[f][0], 0, periodProgress );
				buf[f][1] = m_hp24.update( buf[f][1], 1, periodProgress );
			}

			if( hp48Active )
			{
				buf[f][0] = m_hp480.update( buf[f][0], 0, periodProgress );
				buf[f][1] = m_hp480.update( buf[f][1], 1, periodProgress );

				buf[f][0] = m_hp481.update( buf[f][0], 0, periodProgress );
				buf[f][1] = m_hp481.update( buf[f][1], 1, periodProgress );
			}
		}

		if( lowShelfActive )
		{
			buf[f][0] = m_lowShelf.update( buf[f][0], 0, periodProgress );
			buf[f][1] = m_lowShelf.update( buf[f][1], 1, periodProgress );
		}

		if( para1Active )
		{
			buf[f][0] = m_para1.update( buf[f][0], 0, periodProgress );
			buf[f][1] = m_para1.update( buf[f][1], 1, periodProgress );
		}

		if( para2Active )
		{
			buf[f][0] = m_para2.update( buf[f][0], 0, periodProgress );
			buf[f][1] = m_para2.update( buf[f][1], 1, periodProgress );
		}

		if( para3Active )
		{
			buf[f][0] = m_para3.update( buf[f][0], 0, periodProgress );
			buf[f][1] = m_para3.update( buf[f][1], 1, periodProgress );
		}

		if( para4Active )
		{
			buf[f][0] = m_para4.update( buf[f][0], 0, periodProgress );
			buf[f][1] = m_para4.update( buf[f][1], 1, periodProgress );
		}

		if( highShelfActive )
		{
			buf[f][0] = m_highShelf.update( buf[f][0], 0, periodProgress );
			buf[f][1] = m_highShelf.update( buf[f][1], 1, periodProgress );
		}

		if( lpActive ){
			buf[f][0] = m_lp12.update( buf[f][0], 0, periodProgress );
			buf[f][1] = m_lp12.update( buf[f][1], 1, periodProgress );

			if( lp24Active || lp48Active )
			{
				buf[f][0] = m_lp24.update( buf[f][0], 0, periodProgress );
				buf[f][1] = m_lp24.update( buf[f][1], 1, periodProgress );
			}

			if( lp48Active )
			{
				buf[f][0] = m_lp480.update( buf[f][0], 0, periodProgress );
				buf[f][1] = m_lp480.update( buf[f][1], 1, periodProgress );

				buf[f][0] = m_lp481.update( buf[f][0], 0, periodProgress );
				buf[f][1] = m_lp481.update( buf[f][1], 1, periodProgress );
			}
		}

		//apply wet / dry levels
		buf[f][1] = ( dry * dryS[1] ) + ( wet * buf[f][1] );
		buf[f][0] = ( dry * dryS[0] ) + ( wet * buf[f][0] );


	}

	SampleFrame outPeak = { 0, 0 };
	gain( buf, frames, outGain, &outPeak );
	m_eqControls.m_outPeakL = m_eqControls.m_outPeakL < outPeak[0] ? outPeak[0] : m_eqControls.m_outPeakL;
	m_eqControls.m_outPeakR = m_eqControls.m_outPeakR < outPeak[1] ? outPeak[1] : m_eqControls.m_outPeakR;

	if(m_eqControls.m_analyseOutModel.value( true ) && outSum > 0 && m_eqControls.isViewVisible() )
	{
		m_eqControls.m_outFftBands.analyze( buf, frames );
		setBandPeaks( &m_eqControls.m_outFftBands , ( int )( sampleRate ) );
	}
	else
	{
		m_eqControls.m_outFftBands.clear();
	}

	m_eqControls.m_inProgress = false;

	return Effect::ProcessStatus::ContinueIfNotQuiet;
}




float EqEffect::linearPeakBand(float minF, float maxF, EqAnalyser* fft, int sr)
{
	auto const fftEnergy = fft->getEnergy();
	if (fftEnergy == 0.) { return 0.; }


	float peakLinear = 0.;

	for (int i = 0; i < MAX_BANDS; ++i)
	{
		if (bandToFreq(i, sr) >= minF && bandToFreq(i, sr) <= maxF)
		{
			peakLinear = std::max(peakLinear, fft->m_bands[i] / fftEnergy);
		}
	}

	return peakLinear;
}




void EqEffect::setBandPeaks( EqAnalyser *fft, int samplerate )
{
	auto computePeakBand = [&](const FloatModel& freqModel, const FloatModel& bwModel)
	{
		float const freq = freqModel.value();
		float const bw = bwModel.value();

		return linearPeakBand(freq * (1 - bw * 0.5), freq * (1 + bw * 0.5), fft, samplerate);
	};

	m_eqControls.m_lowShelfPeakR = m_eqControls.m_lowShelfPeakL =
		linearPeakBand(m_eqControls.m_lowShelfFreqModel.value() * (1 - m_eqControls.m_lowShelfResModel.value() * 0.5),
			m_eqControls.m_lowShelfFreqModel.value(), fft , samplerate);

	m_eqControls.m_para1PeakL = m_eqControls.m_para1PeakR =
		computePeakBand(m_eqControls.m_para1FreqModel, m_eqControls.m_para1BwModel);

	m_eqControls.m_para2PeakL = m_eqControls.m_para2PeakR =
		computePeakBand(m_eqControls.m_para2FreqModel, m_eqControls.m_para2BwModel);

	m_eqControls.m_para3PeakL = m_eqControls.m_para3PeakR =
		computePeakBand(m_eqControls.m_para3FreqModel, m_eqControls.m_para3BwModel);

	m_eqControls.m_para4PeakL = m_eqControls.m_para4PeakR =
		computePeakBand(m_eqControls.m_para4FreqModel, m_eqControls.m_para4BwModel);

	m_eqControls.m_highShelfPeakL = m_eqControls.m_highShelfPeakR =
		linearPeakBand(m_eqControls.m_highShelfFreqModel.value(),
			m_eqControls.m_highShelfFreqModel.value() * (1 + m_eqControls.m_highShelfResModel.value() * 0.5),
			fft, samplerate);
}

extern "C"
{

//needed for getting plugin out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model* parent, void* data )
{
	return new EqEffect( parent , static_cast<const Plugin::Descriptor::SubPluginFeatures::Key *>( data ) );
}

}


} // namespace lmms