/*
 * WTSynth.cpp - work in process, name pending
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
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

#include <QtXml/QDomElement>

#include "WTSynth.h"
#include "engine.h"
#include "InstrumentTrack.h"
#include "templates.h"
#include "tooltip.h"
#include "song.h"
#include "lmms_math.h"

#include "embed.cpp"

extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT wtsynth_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"WTSynth",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"4-oscillator modulatable wavetable synth" ),
	"Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>",
	0x0100,
	Plugin::Instrument,
	new PluginPixmapLoader( "logo" ),
	NULL,
	NULL
} ;

}


WTSynthObject::WTSynthObject( float * _A1wave, float * _A2wave,
					float * _B1wave, float * _B2wave,
					int _amod, int _bmod, const sample_rate_t _samplerate, NotePlayHandle * _nph, fpp_t _frames ) :
				m_A1wave( _A1wave ),
				m_A2wave( _A2wave ),
				m_B1wave( _B1wave ),
				m_B2wave( _B2wave ),
				m_amod( _amod ),
				m_bmod( _bmod ),
				m_samplerate( _samplerate ),
				m_nph( _nph ),
				m_fpp( _frames )
{
	m_abuf = new sampleFrame[_frames];
	m_bbuf = new sampleFrame[_frames];
	
	m_lphase[A1_OSC] = 0.0f;
	m_lphase[A2_OSC] = 0.0f;
	m_lphase[B1_OSC] = 0.0f;
	m_lphase[B2_OSC] = 0.0f;
	
	m_rphase[A1_OSC] = 0.0f;
	m_rphase[A2_OSC] = 0.0f;
	m_rphase[B1_OSC] = 0.0f;
	m_rphase[B2_OSC] = 0.0f;
}



WTSynthObject::~WTSynthObject()
{
	delete[] m_abuf;
	delete[] m_bbuf;
}


void WTSynthObject::renderOutput( fpp_t _frames )
{
	if( m_abuf == NULL )
		m_abuf = new sampleFrame[m_fpp];
	if( m_bbuf == NULL )
		m_bbuf = new sampleFrame[m_fpp];
		
	for( fpp_t frame = 0; frame < _frames; frame++ )
	{
		float frac;

		// A2
		frac = fraction( m_lphase[A2_OSC] );
		sample_t A2_L = 
			( m_A2wave[ static_cast<int>( m_lphase[A2_OSC] ) % WAVELEN ] * ( 1.0f - frac ) ) +
			( m_A2wave[ static_cast<int>( m_lphase[A2_OSC] + 1 ) % WAVELEN ] * frac );
		A2_L *= m_lvol[A2_OSC];
		frac = fraction( m_rphase[A2_OSC] );
		sample_t A2_R = 
			( m_A2wave[ static_cast<int>( m_rphase[A2_OSC] ) % WAVELEN ] * ( 1.0f - frac ) ) +
			( m_A2wave[ static_cast<int>( m_rphase[A2_OSC] + 1 ) % WAVELEN ] * frac );
		A2_R *= m_rvol[A2_OSC];

		// B2
		frac = fraction( m_lphase[B2_OSC] );
		sample_t B2_L = 
			( m_B2wave[ static_cast<int>( m_lphase[B2_OSC] ) % WAVELEN ] * ( 1.0f - frac ) ) +
			( m_B2wave[ static_cast<int>( m_lphase[B2_OSC] + 1 ) % WAVELEN ] * frac );
		B2_L *= m_lvol[B2_OSC];
		frac = fraction( m_rphase[B2_OSC] );
		sample_t B2_R = 
			( m_B2wave[ static_cast<int>( m_rphase[B2_OSC] ) % WAVELEN ] * ( 1.0f - frac ) ) +
			( m_B2wave[ static_cast<int>( m_rphase[B2_OSC] + 1 ) % WAVELEN ] * frac );
		B2_R *= m_rvol[B2_OSC];

		// put phases of 1-series oscs into variables because phase modulation might happen
		float A1_lphase = m_lphase[A1_OSC];
		float A1_rphase = m_rphase[A1_OSC];
		float B1_lphase = m_lphase[B1_OSC];
		float B1_rphase = m_rphase[B1_OSC];

		// if phase mod, add to phases
		if( m_amod == MOD_PM )
		{
			A1_lphase = fmodf( A1_lphase + A2_L * PMOD_AMT, WAVELEN );
			while( A1_lphase < 0 ) A1_lphase += WAVELEN;
			A1_rphase = fmodf( A1_rphase + A2_R * PMOD_AMT, WAVELEN );
			while( A1_rphase < 0 ) A1_rphase += WAVELEN;
		}
		if( m_bmod == MOD_PM )
		{
			B1_lphase = fmodf( B1_lphase + B2_L * PMOD_AMT, WAVELEN );
			while( B1_lphase < 0 ) B1_lphase += WAVELEN;
			B1_rphase = fmodf( B1_rphase + B2_R * PMOD_AMT, WAVELEN );
			while( B1_rphase < 0 ) B1_rphase += WAVELEN;
		}

		// A1
		frac = fraction( A1_lphase );
		sample_t A1_L = 
			( m_A1wave[ static_cast<int>( A1_lphase ) % WAVELEN ] * ( 1.0f - frac ) ) +
			( m_A1wave[ static_cast<int>( A1_lphase + 1 ) % WAVELEN ] * frac );
		A1_L *= m_lvol[A1_OSC];
		frac = fraction( A1_rphase );
		sample_t A1_R = 
			( m_A1wave[ static_cast<int>( A1_rphase ) % WAVELEN ] * ( 1.0f - frac ) ) +
			( m_A1wave[ static_cast<int>( A1_rphase + 1 ) % WAVELEN ] * frac );
		A1_R *= m_rvol[A1_OSC];

		// B1
		frac = fraction( B1_lphase );
		sample_t B1_L = 
			( m_B1wave[ static_cast<int>( B1_lphase ) % WAVELEN ] * ( 1.0f - frac ) ) +
			( m_B1wave[ static_cast<int>( B1_lphase + 1 ) % WAVELEN ] * frac );
		B1_L *= m_lvol[B1_OSC];
		frac = fraction( B1_rphase );
		sample_t B1_R = 
			( m_B1wave[ static_cast<int>( B1_rphase ) % WAVELEN ] * ( 1.0f - frac ) ) +
			( m_B1wave[ static_cast<int>( B1_rphase + 1 ) % WAVELEN ] * frac );
		B1_R *= m_rvol[B1_OSC];
		
		// A-series modulation)
		switch( m_amod )
		{
			case MOD_MIX:
				A1_L = ( A1_L + A2_L ) / 2.0;
				A1_R = ( A1_R + A2_R ) / 2.0;
				break;
			case MOD_AM:
				A1_L *= qMax( 0.0f, A2_L + 1.0f );
				A1_R *= qMax( 0.0f, A2_R + 1.0f );
				break;
			case MOD_RM:
				A1_L *= A2_L;
				A1_R *= A2_R;
				break;
		}
		m_abuf[frame][0] = A1_L;
		m_abuf[frame][1] = A1_R;

		// B-series modulation (other than phase mod)
		switch( m_bmod )
		{
			case MOD_MIX:
				B1_L = ( B1_L + B2_L ) / 2.0;
				B1_R = ( B1_R + B2_R ) / 2.0;
				break;
			case MOD_AM:
				B1_L *= qMax( 0.0f, B2_L + 1.0f );
				B1_R *= qMax( 0.0f, B2_R + 1.0f );
				break;
			case MOD_RM:
				B1_L *= B2_L;
				B1_R *= B2_R;
				break;
		}
		m_bbuf[frame][0] = B1_L;
		m_bbuf[frame][1] = B1_R;

		// update phases
		for( int i = 0; i < NUM_OSCS; i++ )
		{
			m_lphase[i] += ( static_cast<float>( WAVELEN ) / ( m_samplerate / ( m_nph->frequency() * m_lfreq[i] ) ) );
			m_lphase[i] = fmodf( m_lphase[i], WAVELEN );
			m_rphase[i] += ( static_cast<float>( WAVELEN ) / ( m_samplerate / ( m_nph->frequency() * m_rfreq[i] ) ) );
			m_rphase[i] = fmodf( m_rphase[i], WAVELEN );
		}
	}

}


void WTSynthObject::updateFrequencies()
{
	// calculate frequencies
	for( int i = 0; i < NUM_OSCS; i++ )
	{
		m_lfreq[i] = ( m_mult[i] / 8 ) * powf( 2, m_ltune[i] / 1200 );
		m_rfreq[i] = ( m_mult[i] / 8 ) * powf( 2, m_rtune[i] / 1200 );
	}
}



void WTSynthObject::changeVolume( int _osc, float _lvol, float _rvol )
{
	m_lvol[_osc] = _lvol / 100.0;
	m_rvol[_osc] = _rvol / 100.0;
	qDebug( "osc %d vol %f %f", _osc, m_lvol[_osc], m_rvol[_osc] );
}


void WTSynthObject::changeMult( int _osc, float _mul )
{
	m_mult[_osc] = _mul;
}


void WTSynthObject::changeTune( int _osc, float _ltune, float _rtune )
{
	m_ltune[_osc] = _ltune;
	m_rtune[_osc] = _rtune;
}



WTSynthInstrument::WTSynthInstrument( InstrumentTrack * _instrument_track ) :
		Instrument( _instrument_track, &wtsynth_plugin_descriptor ),

		a1_vol( 100.0f, 0.0f, 200.0f, 0.1f, this, tr( "Volume A1" ) ),
		a2_vol( 100.0f, 0.0f, 200.0f, 0.1f, this, tr( "Volume A2" ) ),
		b1_vol( 100.0f, 0.0f, 200.0f, 0.1f, this, tr( "Volume B1" ) ),
		b2_vol( 100.0f, 0.0f, 200.0f, 0.1f, this, tr( "Volume B2" ) ),

		a1_pan( 0.0f, -100.0f, 100.0f, 0.1f, this, tr( "Panning A1" ) ),
		a2_pan( 0.0f, -100.0f, 100.0f, 0.1f, this, tr( "Panning A2" ) ),
		b1_pan( 0.0f, -100.0f, 100.0f, 0.1f, this, tr( "Panning B1" ) ),
		b2_pan( 0.0f, -100.0f, 100.0f, 0.1f, this, tr( "Panning B2" ) ),

		a1_mult( 8.0f, 1.0, 16.0, 1.0, this, tr( "Freq. multiplier A1" ) ),
		a2_mult( 8.0f, 1.0, 16.0, 1.0, this, tr( "Freq. multiplier A2" ) ),
		b1_mult( 8.0f, 1.0, 16.0, 1.0, this, tr( "Freq. multiplier B1" ) ),
		b2_mult( 8.0f, 1.0, 16.0, 1.0, this, tr( "Freq. multiplier B2" ) ),

		a1_ltune( 0.0f, -600.0f, 600.0f, 1.0f, this, tr( "Left detune A1" ) ),
		a2_ltune( 0.0f, -600.0f, 600.0f, 1.0f, this, tr( "Left detune A2" ) ),
		b1_ltune( 0.0f, -600.0f, 600.0f, 1.0f, this, tr( "Left detune B1" ) ),
		b2_ltune( 0.0f, -600.0f, 600.0f, 1.0f, this, tr( "Left detune B2" ) ),

		a1_rtune( 0.0f, -600.0f, 600.0f, 1.0f, this, tr( "Right detune A1" ) ),
		a2_rtune( 0.0f, -600.0f, 600.0f, 1.0f, this, tr( "Right detune A2" ) ),
		b1_rtune( 0.0f, -600.0f, 600.0f, 1.0f, this, tr( "Right detune B1" ) ),
		b2_rtune( 0.0f, -600.0f, 600.0f, 1.0f, this, tr( "Right detune B2" ) ),

		a1_graph( -1.0f, 1.0f, WAVELEN, this ),
		a2_graph( -1.0f, 1.0f, WAVELEN, this ),
		b1_graph( -1.0f, 1.0f, WAVELEN, this ),
		b2_graph( -1.0f, 1.0f, WAVELEN, this ),

		m_abmix( 0.0f, -100.0f, 100.0f, 0.1f, this, tr( "A-B Mix" ) ),
		m_amod( 0, 0, 3, this, tr( "A2-A1 modulation" ) ),
		m_bmod( 0, 0, 3, this, tr( "B2-B1 modulation" ) ),

		m_selectedGraph( 0, 0, 3, this, tr( "Selected graph" ) )
{
	connect( &a1_vol, SIGNAL( dataChanged() ), this, SLOT( updateVolumes() ) );
	connect( &a2_vol, SIGNAL( dataChanged() ), this, SLOT( updateVolumes() ) );
	connect( &b1_vol, SIGNAL( dataChanged() ), this, SLOT( updateVolumes() ) );
	connect( &b2_vol, SIGNAL( dataChanged() ), this, SLOT( updateVolumes() ) );

	connect( &a1_pan, SIGNAL( dataChanged() ), this, SLOT( updateVolumes() ) );
	connect( &a2_pan, SIGNAL( dataChanged() ), this, SLOT( updateVolumes() ) );
	connect( &b1_pan, SIGNAL( dataChanged() ), this, SLOT( updateVolumes() ) );
	connect( &b2_pan, SIGNAL( dataChanged() ), this, SLOT( updateVolumes() ) );

	connect( &a1_mult, SIGNAL( dataChanged() ), this, SLOT( updateMult() ) );
	connect( &a2_mult, SIGNAL( dataChanged() ), this, SLOT( updateMult() ) );
	connect( &b1_mult, SIGNAL( dataChanged() ), this, SLOT( updateMult() ) );
	connect( &b2_mult, SIGNAL( dataChanged() ), this, SLOT( updateMult() ) );

	connect( &a1_ltune, SIGNAL( dataChanged() ), this, SLOT( updateTunes() ) );
	connect( &a2_ltune, SIGNAL( dataChanged() ), this, SLOT( updateTunes() ) );
	connect( &b1_ltune, SIGNAL( dataChanged() ), this, SLOT( updateTunes() ) );
	connect( &b2_ltune, SIGNAL( dataChanged() ), this, SLOT( updateTunes() ) );

	connect( &a1_rtune, SIGNAL( dataChanged() ), this, SLOT( updateTunes() ) );
	connect( &a2_rtune, SIGNAL( dataChanged() ), this, SLOT( updateTunes() ) );
	connect( &b1_rtune, SIGNAL( dataChanged() ), this, SLOT( updateTunes() ) );
	connect( &b2_rtune, SIGNAL( dataChanged() ), this, SLOT( updateTunes() ) );

	a1_graph.setWaveToSine();
	a2_graph.setWaveToSine();
	b1_graph.setWaveToSine();
	b2_graph.setWaveToSine();
}


WTSynthInstrument::~WTSynthInstrument()
{
}


void WTSynthInstrument::playNote( NotePlayHandle * _n,
						sampleFrame * _working_buffer )
{
	if ( _n->totalFramesPlayed() == 0 || _n->m_pluginData == NULL )
	{
		WTSynthObject * w = new WTSynthObject( const_cast<float*>( a1_graph.samples() ),
				const_cast<float*>( a2_graph.samples() ),
				const_cast<float*>( b1_graph.samples() ),
				const_cast<float*>( b2_graph.samples() ),
				m_amod.value(), m_bmod.value(),
				engine::mixer()->processingSampleRate(), _n,
				engine::mixer()->framesPerPeriod() );

		w -> changeMult( A1_OSC, a1_mult.value() );
		w -> changeMult( A2_OSC, a2_mult.value() );
		w -> changeMult( B1_OSC, b1_mult.value() );
		w -> changeMult( B2_OSC, b2_mult.value() );

		w -> changeTune( A1_OSC, a1_ltune.value(), a1_rtune.value() );
		w -> changeTune( A2_OSC, a2_ltune.value(), a2_rtune.value() );
		w -> changeTune( B1_OSC, b1_ltune.value(), b1_rtune.value() );
		w -> changeTune( B2_OSC, b2_ltune.value(), b2_rtune.value() );

		w -> changeVolume( A1_OSC, leftCh( a1_vol.value(), a1_pan.value() ), rightCh( a1_vol.value(), a1_pan.value() ) );
		w -> changeVolume( A2_OSC, leftCh( a2_vol.value(), a2_pan.value() ), rightCh( a2_vol.value(), a2_pan.value() ) );
		w -> changeVolume( B1_OSC, leftCh( b1_vol.value(), b1_pan.value() ), rightCh( b1_vol.value(), b1_pan.value() ) );
		w -> changeVolume( B2_OSC, leftCh( b2_vol.value(), b2_pan.value() ), rightCh( b2_vol.value(), b2_pan.value() ) );
		
		w -> updateFrequencies();

		_n->m_pluginData = w;
	}

	const fpp_t frames = _n->framesLeftForCurrentPeriod();

	WTSynthObject * w = static_cast<WTSynthObject *>( _n->m_pluginData );

	// update oscs if needed

	if( m_volChanged )
	{
		w-> changeVolume( A1_OSC, leftCh( a1_vol.value(), a1_pan.value() ), rightCh( a1_vol.value(), a1_pan.value() ) );
		w-> changeVolume( A2_OSC, leftCh( a2_vol.value(), a2_pan.value() ), rightCh( a2_vol.value(), a2_pan.value() ) );
		w-> changeVolume( B1_OSC, leftCh( b1_vol.value(), b1_pan.value() ), rightCh( b1_vol.value(), b1_pan.value() ) );
		w-> changeVolume( B2_OSC, leftCh( b2_vol.value(), b2_pan.value() ), rightCh( b2_vol.value(), b2_pan.value() ) );
		m_volChanged = false;
	}
	if( m_tuneChanged )
	{
		w-> changeTune( A1_OSC, a1_ltune.value(), a1_rtune.value() );
		w-> changeTune( A2_OSC, a2_ltune.value(), a2_rtune.value() );
		w-> changeTune( B1_OSC, b1_ltune.value(), b1_rtune.value() );
		w-> changeTune( B2_OSC, b2_ltune.value(), b2_rtune.value() );
		w-> updateFrequencies();
		m_tuneChanged = false;
	}
	if( m_multChanged )
	{
		w-> changeMult( A1_OSC, a1_mult.value() );
		w-> changeMult( A2_OSC, a2_mult.value() );
		w-> changeMult( B1_OSC, b1_mult.value() );
		w-> changeMult( B2_OSC, b2_mult.value() );
		w-> updateFrequencies();
		m_multChanged = false;
	}
	
	sampleFrame * abuf = w->abuf();
	sampleFrame * bbuf = w->bbuf();

	w-> renderOutput( frames );

	for( fpp_t f=0; f < frames; f++ )
	{
		// get knob values in sample-exact way
		const float bmix = ( ( m_abmix.value( f ) + 100.0 ) / 200.0 );
		const float amix = 1.0 - bmix;
		
		// mix a/b streams according to mixing knob
		_working_buffer[f][0] = ( abuf[f][0] * amix ) +
								( bbuf[f][0] * bmix );
		_working_buffer[f][1] = ( abuf[f][1] * amix ) +
								( bbuf[f][1] * bmix );
	}

	applyRelease( _working_buffer, _n );

	instrumentTrack()->processAudioBuffer( _working_buffer, frames, _n );
}


void WTSynthInstrument::deleteNotePluginData( NotePlayHandle * _n )
{
	delete static_cast<WTSynthObject *>( _n->m_pluginData );
}


void WTSynthInstrument::saveSettings( QDomDocument & _doc,
							QDomElement & _this )
{
	a1_vol.saveSettings( _doc, _this, "a1_vol" );
	a2_vol.saveSettings( _doc, _this, "a2_vol" );
	b1_vol.saveSettings( _doc, _this, "b1_vol" );
	b2_vol.saveSettings( _doc, _this, "b2_vol" );

	a1_pan.saveSettings( _doc, _this, "a1_pan" );
	a2_pan.saveSettings( _doc, _this, "a2_pan" );
	b1_pan.saveSettings( _doc, _this, "b1_pan" );
	b2_pan.saveSettings( _doc, _this, "b2_pan" );

	a1_mult.saveSettings( _doc, _this, "a1_mult" );
	a2_mult.saveSettings( _doc, _this, "a2_mult" );
	b1_mult.saveSettings( _doc, _this, "b1_mult" );
	b2_mult.saveSettings( _doc, _this, "b2_mult" );

	a1_ltune.saveSettings( _doc, _this, "a1_ltune" );
	a2_ltune.saveSettings( _doc, _this, "a2_ltune" );
	b1_ltune.saveSettings( _doc, _this, "b1_ltune" );
	b2_ltune.saveSettings( _doc, _this, "b2_ltune" );

	a1_rtune.saveSettings( _doc, _this, "a1_rtune" );
	a2_rtune.saveSettings( _doc, _this, "a2_rtune" );
	b1_rtune.saveSettings( _doc, _this, "b1_rtune" );
	b2_rtune.saveSettings( _doc, _this, "b2_rtune" );

	// save graphs
	QString sampleString;

	base64::encode( (const char *)a1_graph.samples(), a1_graph.length() * sizeof(float), sampleString );
	_this.setAttribute( "a1_wave", sampleString );
	base64::encode( (const char *)a2_graph.samples(), a2_graph.length() * sizeof(float), sampleString );
	_this.setAttribute( "a2_wave", sampleString );
	base64::encode( (const char *)b1_graph.samples(), b1_graph.length() * sizeof(float), sampleString );
	_this.setAttribute( "b1_wave", sampleString );
	base64::encode( (const char *)b2_graph.samples(), b2_graph.length() * sizeof(float), sampleString );
	_this.setAttribute( "b2_wave", sampleString );

	m_abmix.saveSettings( _doc, _this, "abmix" );
	m_amod.saveSettings( _doc, _this, "amod" );
	m_bmod.saveSettings( _doc, _this, "bmod" );
	m_selectedGraph.saveSettings( _doc, _this, "selgraph" );
}


void WTSynthInstrument::loadSettings( const QDomElement & _this )
{
	a1_vol.loadSettings( _this, "a1_vol" );
	a2_vol.loadSettings( _this, "a2_vol" );
	b1_vol.loadSettings( _this, "b1_vol" );
	b2_vol.loadSettings( _this, "b2_vol" );

	a1_pan.loadSettings( _this, "a1_pan" );
	a2_pan.loadSettings( _this, "a2_pan" );
	b1_pan.loadSettings( _this, "b1_pan" );
	b2_pan.loadSettings( _this, "b2_pan" );

	a1_mult.loadSettings( _this, "a1_mult" );
	a2_mult.loadSettings( _this, "a2_mult" );
	b1_mult.loadSettings( _this, "b1_mult" );
	b2_mult.loadSettings( _this, "b2_mult" );

	a1_ltune.loadSettings( _this, "a1_ltune" );
	a2_ltune.loadSettings( _this, "a2_ltune" );
	b1_ltune.loadSettings( _this, "b1_ltune" );
	b2_ltune.loadSettings( _this, "b2_ltune" );

	a1_rtune.loadSettings( _this, "a1_rtune" );
	a2_rtune.loadSettings( _this, "a2_rtune" );
	b1_rtune.loadSettings( _this, "b1_rtune" );
	b2_rtune.loadSettings( _this, "b2_rtune" );

	// load graphs
	int size = 0;
	char * dst = 0;

	base64::decode( _this.attribute( "a1_wave"), &dst, &size );
	a1_graph.setSamples( (float*) dst );
	base64::decode( _this.attribute( "a2_wave"), &dst, &size );
	a2_graph.setSamples( (float*) dst );
	base64::decode( _this.attribute( "b1_wave"), &dst, &size );
	b1_graph.setSamples( (float*) dst );
	base64::decode( _this.attribute( "b2_wave"), &dst, &size );
	b2_graph.setSamples( (float*) dst );

	delete[] dst;

	m_abmix.loadSettings( _this, "abmix" );
	m_amod.loadSettings( _this, "amod" );
	m_bmod.loadSettings( _this, "bmod" );
	m_selectedGraph.loadSettings( _this, "selgraph" );
}


QString WTSynthInstrument::nodeName() const
{
	return( wtsynth_plugin_descriptor.name );
}


PluginView * WTSynthInstrument::instantiateView( QWidget * _parent )
{
	return( new WTSynthView( this, _parent ) );
}


void WTSynthInstrument::updateVolumes()
{
	m_volChanged = true;
}
void WTSynthInstrument::updateMult()
{
	m_multChanged = true;
}
void WTSynthInstrument::updateTunes()
{
	m_tuneChanged = true;
}


WTSynthView::WTSynthView( Instrument * _instrument,
					QWidget * _parent ) :
	InstrumentView( _instrument, _parent )
{
	setAutoFillBackground( true );
	QPalette pal;

	pal.setBrush( backgroundRole(),	PLUGIN_NAME::getIconPixmap(	"artwork" ) );
	setPalette( pal );

// knobs... lots of em

	makeknob( a1_volKnob, 118, A1ROW, "Volume", "%", "aKnob" )
	makeknob( a2_volKnob, 118, A2ROW, "Volume", "%", "aKnob" )
	makeknob( b1_volKnob, 118, B1ROW, "Volume", "%", "bKnob" )
	makeknob( b2_volKnob, 118, B2ROW, "Volume", "%", "bKnob"  )

	makeknob( a1_panKnob, 142, A1ROW, "Panning", "", "aKnob" )
	makeknob( a2_panKnob, 142, A2ROW, "Panning", "", "aKnob" )
	makeknob( b1_panKnob, 142, B1ROW, "Panning", "", "bKnob"  )
	makeknob( b2_panKnob, 142, B2ROW, "Panning", "", "bKnob"  )

	makeknob( a1_multKnob, 172, A1ROW, "Freq. multiplier", "/8", "aKnob" )
	makeknob( a2_multKnob, 172, A2ROW, "Freq. multiplier", "/8", "aKnob" )
	makeknob( b1_multKnob, 172, B1ROW, "Freq. multiplier", "/8", "bKnob"  )
	makeknob( b2_multKnob, 172, B2ROW, "Freq. multiplier", "/8", "bKnob"  )

	makeknob( a1_ltuneKnob, 200, A1ROW, "Left detune", " cents", "aKnob" )
	makeknob( a2_ltuneKnob, 200, A2ROW, "Left detune", " cents", "aKnob" )
	makeknob( b1_ltuneKnob, 200, B1ROW, "Left detune", " cents", "bKnob"  )
	makeknob( b2_ltuneKnob, 200, B2ROW, "Left detune", " cents", "bKnob"  )

	makeknob( a1_rtuneKnob, 224, A1ROW, "Right detune", " cents", "aKnob" )
	makeknob( a2_rtuneKnob, 224, A2ROW, "Right detune", " cents", "aKnob" )
	makeknob( b1_rtuneKnob, 224, B1ROW, "Right detune", " cents", "bKnob"  )
	makeknob( b2_rtuneKnob, 224, B2ROW, "Right detune", " cents", "bKnob"  )

	makeknob( m_abmixKnob, 4, 4, "A-B Mix", "", "mixKnob" )

// let's set volume knobs
	a1_volKnob -> setVolumeKnob( true );
	a2_volKnob -> setVolumeKnob( true );
	b1_volKnob -> setVolumeKnob( true );
	b2_volKnob -> setVolumeKnob( true );

	m_abmixKnob -> setFixedSize( 31, 31 );


// button groups next.
// graph select buttons
	pixmapButton * a1_selectButton = new pixmapButton( this, NULL );
	a1_selectButton -> move( 4, 121 );
	a1_selectButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "a1_active" ) );
	a1_selectButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "a1_inactive" ) );
	toolTip::add( a1_selectButton, tr( "Select oscillator A1") );

	pixmapButton * a2_selectButton = new pixmapButton( this, NULL );
	a2_selectButton -> move( 44, 121 );
	a2_selectButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "a2_active" ) );
	a2_selectButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "a2_inactive" ) );
	toolTip::add( a2_selectButton, tr( "Select oscillator A2") );

	pixmapButton * b1_selectButton = new pixmapButton( this, NULL );
	b1_selectButton -> move( 84, 121 );
	b1_selectButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "b1_active" ) );
	b1_selectButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "b1_inactive" ) );
	toolTip::add( b1_selectButton, tr( "Select oscillator B1") );

	pixmapButton * b2_selectButton = new pixmapButton( this, NULL );
	b2_selectButton -> move( 124, 121 );
	b2_selectButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "b2_active" ) );
	b2_selectButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "b2_inactive" ) );
	toolTip::add( b2_selectButton, tr( "Select oscillator B2") );

	m_selectedGraphGroup = new automatableButtonGroup( this );
	m_selectedGraphGroup -> addButton( a1_selectButton );
	m_selectedGraphGroup -> addButton( a2_selectButton );
	m_selectedGraphGroup -> addButton( b1_selectButton );
	m_selectedGraphGroup -> addButton( b2_selectButton );

// A-modulation button group
	pixmapButton * amod_mixButton = new pixmapButton( this, NULL );
	amod_mixButton -> move( 4, 50 );
	amod_mixButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "amix_active" ) );
	amod_mixButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "amix_inactive" ) );
	toolTip::add( amod_mixButton, tr( "Mix output of A2 to A1" ) );

	pixmapButton * amod_amButton = new pixmapButton( this, NULL );
	amod_amButton -> move( 4, 66 );
	amod_amButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "aam_active" ) );
	amod_amButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "aam_inactive" ) );
	toolTip::add( amod_amButton, tr( "Modulate amplitude of A1 with output of A2" ) );

	pixmapButton * amod_rmButton = new pixmapButton( this, NULL );
	amod_rmButton -> move( 4, 82 );
	amod_rmButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "arm_active" ) );
	amod_rmButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "arm_inactive" ) );
	toolTip::add( amod_rmButton, tr( "Ring-modulate A1 and A2" ) );

	pixmapButton * amod_pmButton = new pixmapButton( this, NULL );
	amod_pmButton -> move( 4, 98 );
	amod_pmButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "apm_active" ) );
	amod_pmButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "apm_inactive" ) );
	toolTip::add( amod_pmButton, tr( "Modulate phase of A1 with output of A2" ) );

	m_aModGroup = new automatableButtonGroup( this );
	m_aModGroup -> addButton( amod_mixButton );
	m_aModGroup -> addButton( amod_amButton );
	m_aModGroup -> addButton( amod_rmButton );
	m_aModGroup -> addButton( amod_pmButton );

// B-modulation button group
	pixmapButton * bmod_mixButton = new pixmapButton( this, NULL );
	bmod_mixButton -> move( 44, 50 );
	bmod_mixButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "bmix_active" ) );
	bmod_mixButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "bmix_inactive" ) );
	toolTip::add( bmod_mixButton, tr( "Mix output of B2 to B1" ) );

	pixmapButton * bmod_amButton = new pixmapButton( this, NULL );
	bmod_amButton -> move( 44, 66 );
	bmod_amButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "bam_active" ) );
	bmod_amButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "bam_inactive" ) );
	toolTip::add( bmod_amButton, tr( "Modulate amplitude of B1 with output of B2" ) );

	pixmapButton * bmod_rmButton = new pixmapButton( this, NULL );
	bmod_rmButton -> move( 44, 82 );
	bmod_rmButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "brm_active" ) );
	bmod_rmButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "brm_inactive" ) );
	toolTip::add( bmod_rmButton, tr( "Ring-modulate B1 and B2" ) );

	pixmapButton * bmod_pmButton = new pixmapButton( this, NULL );
	bmod_pmButton -> move( 44, 98 );
	bmod_pmButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "bpm_active" ) );
	bmod_pmButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "bpm_inactive" ) );
	toolTip::add( bmod_pmButton, tr( "Modulate phase of B1 with output of B2" ) );

	m_bModGroup = new automatableButtonGroup( this );
	m_bModGroup -> addButton( bmod_mixButton );
	m_bModGroup -> addButton( bmod_amButton );
	m_bModGroup -> addButton( bmod_rmButton );
	m_bModGroup -> addButton( bmod_pmButton );


// graph widgets
	pal = QPalette();
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap("wavegraph") );
// a1 graph
	a1_graph = new graph( this, graph::LinearStyle, 224, 105 );
	a1_graph->move( 4, 141 );
	a1_graph->setAutoFillBackground( true );
	a1_graph->setGraphColor( QColor( 0x43, 0xb2, 0xff ) );
	toolTip::add( a1_graph, tr ( "Draw your own waveform here by dragging your mouse on this graph." ) );
	a1_graph->setPalette( pal );

// a2 graph
	a2_graph = new graph( this, graph::LinearStyle, 224, 105 );
	a2_graph->move( 4, 141 );
	a2_graph->setAutoFillBackground( true );
	a2_graph->setGraphColor( QColor( 0x43, 0xb2, 0xff ) );
	toolTip::add( a2_graph, tr ( "Draw your own waveform here by dragging your mouse on this graph." ) );
	a2_graph->setPalette( pal );

// b1 graph
	b1_graph = new graph( this, graph::LinearStyle, 224, 105 );
	b1_graph->move( 4, 141 );
	b1_graph->setAutoFillBackground( true );
	b1_graph->setGraphColor( QColor( 0xfc, 0x54, 0x31 ) );
	toolTip::add( b1_graph, tr ( "Draw your own waveform here by dragging your mouse on this graph." ) );
	b1_graph->setPalette( pal );

// b2 graph
	b2_graph = new graph( this, graph::LinearStyle, 224, 105 );
	b2_graph->move( 4, 141 );
	b2_graph->setAutoFillBackground( true );
	b2_graph->setGraphColor( QColor( 0xfc, 0x54, 0x31 ) );
	toolTip::add( b2_graph, tr ( "Draw your own waveform here by dragging your mouse on this graph." ) );
	b2_graph->setPalette( pal );


// misc pushbuttons
// waveform modifications

	m_phaseLeftButton = new pixmapButton( this, tr( "Phase left" ) );
	m_phaseLeftButton -> move ( 200, 121 );
	m_phaseLeftButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "phl_active" ) );
	m_phaseLeftButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "phl_inactive" ) );
	toolTip::add( m_phaseLeftButton, tr( "Click to shift phase by -15 degrees" ) );

	m_phaseRightButton = new pixmapButton( this, tr( "Phase right" ) );
	m_phaseRightButton -> move ( 216, 121 );
	m_phaseRightButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "phr_active" ) );
	m_phaseRightButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "phr_inactive" ) );
	toolTip::add( m_phaseRightButton, tr( "Click to shift phase by +15 degrees" ) );

	m_normalizeButton = new pixmapButton( this, tr( "Normalize" ) );
	m_normalizeButton -> move ( 232, 121 );
	m_normalizeButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "norm_active" ) );
	m_normalizeButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "norm_inactive" ) );
	toolTip::add( m_normalizeButton, tr( "Click to normalize" ) );


	m_invertButton = new pixmapButton( this, tr( "Invert" ) );
	m_invertButton -> move ( 232, 138 );
	m_invertButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "inv_active" ) );
	m_invertButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "inv_inactive" ) );
	toolTip::add( m_invertButton, tr( "Click to invert" ) );

	m_smoothButton = new pixmapButton( this, tr( "Smooth" ) );
	m_smoothButton -> move ( 232, 155 );
	m_smoothButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "smooth_active" ) );
	m_smoothButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "smooth_inactive" ) );
	toolTip::add( m_smoothButton, tr( "Click to smooth" ) );

// waveforms

	m_sinWaveButton = new pixmapButton( this, tr( "Sine wave" ) );
	m_sinWaveButton -> move ( 232, 176 );
	m_sinWaveButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "sin_active" ) );
	m_sinWaveButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "sin_inactive" ) );
	toolTip::add( m_sinWaveButton, tr( "Click for sine wave" ) );

	m_triWaveButton = new pixmapButton( this, tr( "Triangle wave" ) );
	m_triWaveButton -> move ( 232, 194 );
	m_triWaveButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "tri_active" ) );
	m_triWaveButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "tri_inactive" ) );
	toolTip::add( m_triWaveButton, tr( "Click for triangle wave" ) );

	m_sawWaveButton = new pixmapButton( this, tr( "Triangle wave" ) );
	m_sawWaveButton -> move ( 232, 212 );
	m_sawWaveButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "saw_active" ) );
	m_sawWaveButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "saw_inactive" ) );
	toolTip::add( m_sawWaveButton, tr( "Click for saw wave" ) );

	m_sqrWaveButton = new pixmapButton( this, tr( "Square wave" ) );
	m_sqrWaveButton -> move ( 232, 230 );
	m_sqrWaveButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "sqr_active" ) );
	m_sqrWaveButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "sqr_inactive" ) );
	toolTip::add( m_sqrWaveButton, tr( "Click for square wave" ) );



	connect( m_sinWaveButton, SIGNAL( clicked() ), this, SLOT( sinWaveClicked() ) );
	connect( m_triWaveButton, SIGNAL( clicked() ), this, SLOT( triWaveClicked() ) );
	connect( m_sawWaveButton, SIGNAL( clicked() ), this, SLOT( sawWaveClicked() ) );
	connect( m_sqrWaveButton, SIGNAL( clicked() ), this, SLOT( sqrWaveClicked() ) );
	connect( m_normalizeButton, SIGNAL( clicked() ), this, SLOT( normalizeClicked() ) );
	connect( m_invertButton, SIGNAL( clicked() ), this, SLOT( invertClicked() ) );
	connect( m_smoothButton, SIGNAL( clicked() ), this, SLOT( smoothClicked() ) );
	connect( m_phaseLeftButton, SIGNAL( clicked() ), this, SLOT( phaseLeftClicked() ) );
	connect( m_phaseRightButton, SIGNAL( clicked() ), this, SLOT( phaseRightClicked() ) );

	connect( a1_selectButton, SIGNAL( clicked() ), this, SLOT( updateLayout() ) );
	connect( a2_selectButton, SIGNAL( clicked() ), this, SLOT( updateLayout() ) );
	connect( b1_selectButton, SIGNAL( clicked() ), this, SLOT( updateLayout() ) );
	connect( b2_selectButton, SIGNAL( clicked() ), this, SLOT( updateLayout() ) );

	updateLayout();
}


WTSynthView::~WTSynthView()
{
}



void WTSynthView::updateLayout()
{
	switch( m_selectedGraphGroup->model()->value() )
	{
		case A1_OSC:
			a1_graph->show();
			a2_graph->hide();
			b1_graph->hide();
			b2_graph->hide();
			break;
		case A2_OSC:
			a1_graph->hide();
			a2_graph->show();
			b1_graph->hide();
			b2_graph->hide();
			break;
		case B1_OSC:
			a1_graph->hide();
			a2_graph->hide();
			b1_graph->show();
			b2_graph->hide();
			break;
		case B2_OSC:
			a1_graph->hide();
			a2_graph->hide();
			b1_graph->hide();
			b2_graph->show();
			break;
	}
}



void WTSynthView::sinWaveClicked()
{
	switch( m_selectedGraphGroup->model()->value() )
	{
		case A1_OSC:
			a1_graph->model()->setWaveToSine();
			engine::getSong()->setModified();
			break;
		case A2_OSC:
			a2_graph->model()->setWaveToSine();
			engine::getSong()->setModified();
			break;
		case B1_OSC:
			b1_graph->model()->setWaveToSine();
			engine::getSong()->setModified();
			break;
		case B2_OSC:
			b2_graph->model()->setWaveToSine();
			engine::getSong()->setModified();
			break;
	}
}


void WTSynthView::triWaveClicked()
{
	switch( m_selectedGraphGroup->model()->value() )
	{
		case A1_OSC:
			a1_graph->model()->setWaveToTriangle();
			engine::getSong()->setModified();
			break;
		case A2_OSC:
			a2_graph->model()->setWaveToTriangle();
			engine::getSong()->setModified();
			break;
		case B1_OSC:
			b1_graph->model()->setWaveToTriangle();
			engine::getSong()->setModified();
			break;
		case B2_OSC:
			b2_graph->model()->setWaveToTriangle();
			engine::getSong()->setModified();
			break;
	}
}


void WTSynthView::sawWaveClicked()
{
	switch( m_selectedGraphGroup->model()->value() )
	{
		case A1_OSC:
			a1_graph->model()->setWaveToSaw();
			engine::getSong()->setModified();
			break;
		case A2_OSC:
			a2_graph->model()->setWaveToSaw();
			engine::getSong()->setModified();
			break;
		case B1_OSC:
			b1_graph->model()->setWaveToSaw();
			engine::getSong()->setModified();
			break;
		case B2_OSC:
			b2_graph->model()->setWaveToSaw();
			engine::getSong()->setModified();
			break;
	}
}


void WTSynthView::sqrWaveClicked()
{
	switch( m_selectedGraphGroup->model()->value() )
	{
		case A1_OSC:
			a1_graph->model()->setWaveToSquare();
			engine::getSong()->setModified();
			break;
		case A2_OSC:
			a2_graph->model()->setWaveToSquare();
			engine::getSong()->setModified();
			break;
		case B1_OSC:
			b1_graph->model()->setWaveToSquare();
			engine::getSong()->setModified();
			break;
		case B2_OSC:
			b2_graph->model()->setWaveToSquare();
			engine::getSong()->setModified();
			break;
	}
}


void WTSynthView::normalizeClicked()
{
	switch( m_selectedGraphGroup->model()->value() )
	{
		case A1_OSC:
			a1_graph->model()->normalize();
			engine::getSong()->setModified();
			break;
		case A2_OSC:
			a2_graph->model()->normalize();
			engine::getSong()->setModified();
			break;
		case B1_OSC:
			b1_graph->model()->normalize();
			engine::getSong()->setModified();
			break;
		case B2_OSC:
			b2_graph->model()->normalize();
			engine::getSong()->setModified();
			break;
	}
}


void WTSynthView::invertClicked()
{
	switch( m_selectedGraphGroup->model()->value() )
	{
		case A1_OSC:
			a1_graph->model()->invert();
			engine::getSong()->setModified();
			break;
		case A2_OSC:
			a2_graph->model()->invert();
			engine::getSong()->setModified();
			break;
		case B1_OSC:
			b1_graph->model()->invert();
			engine::getSong()->setModified();
			break;
		case B2_OSC:
			b2_graph->model()->invert();
			engine::getSong()->setModified();
			break;
	}
}


void WTSynthView::smoothClicked()
{
	switch( m_selectedGraphGroup->model()->value() )
	{
		case A1_OSC:
			a1_graph->model()->smooth();
			engine::getSong()->setModified();
			break;
		case A2_OSC:
			a2_graph->model()->smooth();
			engine::getSong()->setModified();
			break;
		case B1_OSC:
			b1_graph->model()->smooth();
			engine::getSong()->setModified();
			break;
		case B2_OSC:
			b2_graph->model()->smooth();
			engine::getSong()->setModified();
			break;
	}
}


void WTSynthView::phaseLeftClicked()
{
	switch( m_selectedGraphGroup->model()->value() )
	{
		case A1_OSC:
			a1_graph->model()->shiftPhase( -15 );
			engine::getSong()->setModified();
			break;
		case A2_OSC:
			a2_graph->model()->shiftPhase( -15 );
			engine::getSong()->setModified();
			break;
		case B1_OSC:
			b1_graph->model()->shiftPhase( -15 );
			engine::getSong()->setModified();
			break;
		case B2_OSC:
			b2_graph->model()->shiftPhase( -15 );
			engine::getSong()->setModified();
			break;
	}
}


void WTSynthView::phaseRightClicked()
{
	switch( m_selectedGraphGroup->model()->value() )
	{
		case A1_OSC:
			a1_graph->model()->shiftPhase( 15 );
			engine::getSong()->setModified();
			break;
		case A2_OSC:
			a2_graph->model()->shiftPhase( 15 );
			engine::getSong()->setModified();
			break;
		case B1_OSC:
			b1_graph->model()->shiftPhase( 15 );
			engine::getSong()->setModified();
			break;
		case B2_OSC:
			b2_graph->model()->shiftPhase( 15 );
			engine::getSong()->setModified();
			break;
	}
}


void WTSynthView::modelChanged()
{
	WTSynthInstrument * w = castModel<WTSynthInstrument>();

	a1_volKnob -> setModel( &w -> a1_vol );
	a2_volKnob -> setModel( &w -> a2_vol );
	b1_volKnob -> setModel( &w -> b1_vol );
	b2_volKnob -> setModel( &w -> b2_vol );

	a1_panKnob -> setModel( &w -> a1_pan );
	a2_panKnob -> setModel( &w -> a2_pan );
	b1_panKnob -> setModel( &w -> b1_pan );
	b2_panKnob -> setModel( &w -> b2_pan );

	a1_multKnob -> setModel( &w -> a1_mult );
	a2_multKnob -> setModel( &w -> a2_mult );
	b1_multKnob -> setModel( &w -> b1_mult );
	b2_multKnob -> setModel( &w -> b2_mult );

	a1_ltuneKnob -> setModel( &w -> a1_ltune );
	a2_ltuneKnob -> setModel( &w -> a2_ltune );
	b1_ltuneKnob -> setModel( &w -> b1_ltune );
	b2_ltuneKnob -> setModel( &w -> b2_ltune );

	a1_rtuneKnob -> setModel( &w -> a1_rtune );
	a2_rtuneKnob -> setModel( &w -> a2_rtune );
	b1_rtuneKnob -> setModel( &w -> b1_rtune );
	b2_rtuneKnob -> setModel( &w -> b2_rtune );

	m_abmixKnob -> setModel( &w -> m_abmix );

	m_selectedGraphGroup -> setModel( &w -> m_selectedGraph );

	m_aModGroup -> setModel( &w -> m_amod );
	m_bModGroup -> setModel( &w -> m_bmod );

	a1_graph -> setModel( &w -> a1_graph );
	a2_graph -> setModel( &w -> a2_graph );
	b1_graph -> setModel( &w -> b1_graph );
	b2_graph -> setModel( &w -> b2_graph );

}





extern "C"
{

// necessary for getting instance out of shared lib
Plugin * PLUGIN_EXPORT lmms_plugin_main( Model *, void * _data )
{
	return( new WTSynthInstrument( static_cast<InstrumentTrack *>( _data ) ) );
}


}


#include "moc_WTSynth.cxx"
