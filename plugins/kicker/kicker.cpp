/*
 * kicker.cpp - bassdrum-synthesizer
 *
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "qt3support.h"

#ifdef QT4

#include <QtGui/QLayout>
#include <Qt/QtXml>

#else

#include <qdom.h>
#include <qlayout.h>

#endif


#include "kicker.h"
#include "instrument_track.h"
#include "note_play_handle.h"
#include "sweep_oscillator.h"
#include "buffer_allocator.h"
#include "knob.h"

#undef SINGLE_SOURCE_COMPILE
#include "embed.cpp"


extern "C"
{

plugin::descriptor kicker_plugin_descriptor =
{
	STRINGIFY_PLUGIN_NAME( PLUGIN_NAME ),
	"Kicker",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"versatile kick- & bassdrum-synthesizer" ),
	"Tobias Doerffel <tobydox/at/users.sf.net>",
	0x0100,
	plugin::Instrument,
	new QPixmap( PLUGIN_NAME::getIconPixmap( "logo" ) ),
	NULL
} ;

}


kickerInstrument::kickerInstrument( instrumentTrack * _instrument_track ) :
	instrument( _instrument_track, &kicker_plugin_descriptor )
{
	QVBoxLayout * vl = new QVBoxLayout( this );
	QHBoxLayout * hl = new QHBoxLayout;
	m_startFreqKnob = new knob( knobDark_28, this, tr( "Start frequency" ),
						eng(), _instrument_track );
	m_startFreqKnob->setRange( 5.0f, 1000.0f, 1.0f );
 	m_startFreqKnob->setInitValue( 150.0f );
	m_startFreqKnob->setLabel( tr( "START" ) );
	m_startFreqKnob->setHintText( tr( "Start frequency:" ) + " ", "Hz" );

	m_endFreqKnob = new knob( knobDark_28, this, tr( "End frequency" ),
						eng(), _instrument_track );
	m_endFreqKnob->setRange( 5.0f, 1000.0f, 1.0f );
	m_endFreqKnob->setInitValue( 40.0f );
	m_endFreqKnob->setLabel( tr( "END" ) );
	m_endFreqKnob->setHintText( tr( "End frequency:" ) + " ", "Hz" );

	m_decayKnob = new knob( knobDark_28, this, tr( "Decay" ),
						eng(), _instrument_track );
	m_decayKnob->setRange( 5.0f, 1000.0f, 1.0f );
	m_decayKnob->setInitValue( 120.0f );
	m_decayKnob->setLabel( tr( "DECAY" ) );
	m_decayKnob->setHintText( tr( "Decay:" ) + " ", "ms" );

	m_distKnob = new knob( knobDark_28, this, tr( "Distortion" ),
						eng(), _instrument_track );
	m_distKnob->setRange( 0.0f, 100.0f, 0.1f );
	m_distKnob->setInitValue( 0.8f );
	m_distKnob->setLabel( tr( "DIST" ) );
	m_distKnob->setHintText( tr( "Distortion:" ) + " ", "" );

	m_gainKnob = new knob( knobDark_28, this, tr( "Gain" ),
						eng(), _instrument_track );
	m_gainKnob->setRange( 0.1f, 5.0f, 0.05f );
	m_gainKnob->setInitValue( 1.0f );
	m_gainKnob->setLabel( tr( "GAIN" ) );
	m_gainKnob->setHintText( tr( "Gain:" ) + " ", "" );

	hl->addWidget( m_startFreqKnob );
	hl->addWidget( m_endFreqKnob );
	hl->addWidget( m_decayKnob );
	hl->addWidget( m_distKnob );
	hl->addWidget( m_gainKnob );

	vl->addLayout( hl );
#ifdef QT4
	setAutoFillBackground( TRUE );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap(
								"artwork" ) );
	setPalette( pal );
#else
	setErasePixmap( PLUGIN_NAME::getIconPixmap( "artwork" ) );
#endif
}




kickerInstrument::~kickerInstrument()
{
}




void kickerInstrument::saveSettings( QDomDocument & _doc,
							QDomElement & _this )
{
	m_startFreqKnob->saveSettings( _doc, _this, "startfreq" );
	m_endFreqKnob->saveSettings( _doc, _this, "endfreq" );
	m_decayKnob->saveSettings( _doc, _this, "decay" );
	m_distKnob->saveSettings( _doc, _this, "dist" );
	m_gainKnob->saveSettings( _doc, _this, "gain" );
}




void kickerInstrument::loadSettings( const QDomElement & _this )
{
	m_startFreqKnob->loadSettings( _this, "startfreq" );
	m_endFreqKnob->loadSettings( _this, "endfreq" );
	m_decayKnob->loadSettings( _this, "decay" );
	m_distKnob->loadSettings( _this, "dist" );
	m_gainKnob->loadSettings( _this, "gain" );
}




QString kickerInstrument::nodeName( void ) const
{
	return( kicker_plugin_descriptor.name );
}



//typedef effectLib::foldbackDistortion<> distFX;
typedef effectLib::distortion<> distFX;
typedef sweepOscillator<effectLib::monoToStereoAdaptor<distFX> > sweepOsc;


void kickerInstrument::playNote( notePlayHandle * _n, bool )
{
	const float decfr = m_decayKnob->value() *
				eng()->getMixer()->sampleRate() / 1000.0f;
	const f_cnt_t tfp = _n->totalFramesPlayed();

	if ( tfp == 0 )
	{
		_n->m_pluginData = new sweepOsc(
					distFX( m_distKnob->value(),
							m_gainKnob->value() ) );
	}
	else if( tfp > decfr && !_n->released() )
	{
		_n->noteOff();
	}

	//const float freq = getInstrumentTrack()->frequency( _n ) / 2;
	const float fdiff = m_endFreqKnob->value() - m_startFreqKnob->value();
	const fpab_t frames = _n->released() ?
		tMax( tMin<f_cnt_t>( desiredReleaseFrames() -
							_n->releaseFramesDone(),
				eng()->getMixer()->framesPerAudioBuffer() ), 0 )
		:
		eng()->getMixer()->framesPerAudioBuffer();
	const float f1 = m_startFreqKnob->value() + tfp * fdiff / decfr;
	const float f2 = m_startFreqKnob->value() + (frames+tfp-1)*fdiff/decfr;

	sampleFrame * buf = bufferAllocator::alloc<sampleFrame>( frames );


	sweepOsc * so = static_cast<sweepOsc *>( _n->m_pluginData );
	so->update( buf, frames, f1, f2, eng()->getMixer()->sampleRate() );

	if( _n->released() )
	{
		for( fpab_t f = 0; f < frames; ++f )
		{
			const float fac = 1.0f -
				(float)( _n->releaseFramesDone()+f ) /
							desiredReleaseFrames();
			for( ch_cnt_t ch = 0; ch < DEFAULT_CHANNELS; ++ch )
			{
				buf[f][ch] *= fac;
			}
		}
	}

	getInstrumentTrack()->processAudioBuffer( buf, frames, _n );

	bufferAllocator::free( buf );
}



void kickerInstrument::deleteNotePluginData( notePlayHandle * _n )
{
	delete static_cast<sweepOsc *>( _n->m_pluginData );
}







extern "C"
{

// neccessary for getting instance out of shared lib
plugin * lmms_plugin_main( void * _data )
{
	return( new kickerInstrument(
				static_cast<instrumentTrack *>( _data ) ) );
}


}


