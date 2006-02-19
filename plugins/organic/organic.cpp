/*
 * bit_invader.cpp - instrument which uses a usereditable wavetable
 *
 * Copyright (c) 2006 Andreas Brandmaier <andy/at/brandmaier/dot/de>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#include "qt3support.h"

#ifdef QT4

#include <QPainter>
#include <QButtonGroup>
#include <QBitmap>
#include <Qt/QtXml>
#include <QFileInfo>
#include <QDropEvent>

#else

#include <qpainter.h>
#include <qbuttongroup.h>
#include <qbitmap.h>
#include <qdom.h>
#include <qfileinfo.h>
#include <qdom.h>
#include <qmap.h>
#include <qcanvas.h>
#include <qlabel.h>

#endif

#include <iostream>
#include <cstdlib>
#include <ctime>
#include "math.h"

using namespace std;


#include "organic.h"
#include "channel_track.h"
#include "note_play_handle.h"
#include "templates.h"
#include "buffer_allocator.h"
#include "knob.h"
#include "pixmap_button.h"
#include "tooltip.h"
#include "song_editor.h"
#include "oscillator.h"
#include "sample_buffer.h"
#include "embed.cpp"
#include "base64.h"


extern "C"
{

plugin::descriptor organic_plugin_descriptor =
{
	STRINGIFY_PLUGIN_NAME( PLUGIN_NAME ),
	"Organic",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"Additive Synthesizer for organ-like sounds" ),
	"Andreas Brandmaier <andreas/at/brandmaier.de>",
	0x0100,
	plugin::INSTRUMENT,
	PLUGIN_NAME::findEmbeddedData( "logo.png" )
} ;

}

QPixmap * organicInstrument::s_artwork = NULL;


/***********************************************************************
*
*	class BitInvader
*
*	lmms - plugin 
*
***********************************************************************/


organicInstrument::organicInstrument( channelTrack * _channel_track ) :
	instrument( _channel_track,
			&organic_plugin_descriptor ),
	specialBgHandlingWidget( PLUGIN_NAME::getIconPixmap( "artwork" ) )
{


	m_num_oscillators = 8;
	
	m_osc = new oscillatorData[m_num_oscillators];
	
	for (int i=0; i < m_num_oscillators; i++)
	{
		m_osc[i].waveShape = oscillator::SIN_WAVE;

		// setup volume-knob
		m_osc[i].oscKnob = new knob( knobGreen_17, this, tr(
					"Osc %1 waveform" ).arg( i+1 ), eng() );
		m_osc[i].oscKnob->move( 25+i*20, 90 );
		m_osc[i].oscKnob->setRange( 0.0f, 5.0f, 0.25f );
		m_osc[i].oscKnob->setValue( 0.0f, TRUE );
		m_osc[i].oscKnob->setHintText( tr( "Osc %1 waveform:" ).arg(
							i+1 ) + " ", "%" );
										
		connect( m_osc[i].oscKnob, SIGNAL( valueChanged( float ) ),
			this, SLOT (oscButtonChanged( void ) ) 
		);
										
		// setup volume-knob
		m_osc[i].volKnob = new knob( knobGreen_17, this, tr(
					"Osc %1 volume" ).arg( i+1 ), eng() );
		m_osc[i].volKnob->move( 25+i*20, 110 );
		m_osc[i].volKnob->setRange( 0, 100, 1.0f );
		m_osc[i].volKnob->setValue( 100, TRUE );
		m_osc[i].volKnob->setHintText( tr( "Osc %1 volume:" ).arg(
							i+1 ) + " ", "%" );
							
		// setup panning-knob
		m_osc[i].panKnob = new knob( knobGreen_17, this,
				tr( "Osc %1 panning" ).arg( i + 1 ), eng() );
		m_osc[i].panKnob->move( 25+i*20, 130 );
		m_osc[i].panKnob->setRange( PANNING_LEFT, PANNING_RIGHT, 1.0f );
		m_osc[i].panKnob->setValue( DEFAULT_PANNING, TRUE );
		m_osc[i].panKnob->setHintText( tr("Osc %1 panning:").arg( i+1 )
						+ " ", "" );
							
		// setup knob for left fine-detuning
		m_osc[i].detuneKnob = new knob( knobGreen_17, this,
				tr( "Osc %1 fine detuning left" ).arg( i+1 ),
									eng() );
		m_osc[i].detuneKnob->move( 25+i*20, 150 );
		m_osc[i].detuneKnob->setRange( -100.0f, 100.0f, 1.0f );
		m_osc[i].detuneKnob->setValue( 0.0f, TRUE );
		m_osc[i].detuneKnob->setHintText( tr( "Osc %1 fine detuning "
							"left:" ).arg( i + 1 )
							+ " ", " " +
							tr( "cents" ) );
							
						
							
	}

		// setup knob for FX1
		fx1Knob = new knob( knobGreen_17, this,
				tr( "FX1" ),
									eng() );
		fx1Knob->move( 20, 200 );
		fx1Knob->setRange( 0.0f, 0.99f, 0.01f );
		fx1Knob->setValue( 0.0f, TRUE );
		
		// setup volume-knob
		volKnob = new knob( knobGreen_17, this, tr(
					"Osc %1 volume" ).arg( 1 ), eng() );
		volKnob->move( 50, 200 );
		volKnob->setRange( 0, 200, 1.0f );
		volKnob->setValue( 100, TRUE );
		volKnob->setHintText( tr( "Osc %1 volume:" ).arg(
							1 ) + " ", "%" );


	m_osc[0].harmonic = 0.5f;	//	one octave below
	m_osc[1].harmonic = 0.75f;	//	a fifth below
	m_osc[2].harmonic = 1.0f;	// base freq
	m_osc[3].harmonic = 2.0f;	// first overtone
	m_osc[4].harmonic = 3.0f;	// second overtone
	m_osc[5].harmonic = 4.0f;	// .
	m_osc[6].harmonic = 5.0f;	// .
	m_osc[7].harmonic = 6.0f;	// .

			
	if( s_artwork == NULL )
	{
		s_artwork = new QPixmap( PLUGIN_NAME::getIconPixmap(
								"artwork" ) );
	}



#ifdef QT4
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap(
								"artwork" ) );
	setPalette( pal );
#else
	setErasePixmap( PLUGIN_NAME::getIconPixmap( "artwork" ) );
#endif
}



organicInstrument::~organicInstrument()
{
}




void organicInstrument::saveSettings( QDomDocument & _doc,
							QDomElement & _parent )
{

	QDomElement to_de = _doc.createElement( nodeName() );

	to_de.setAttribute( "num_osc", QString::number( m_num_oscillators ) );
	to_de.setAttribute( "foldback", QString::number( fx1Knob->value() ) );
	to_de.setAttribute( "vol", QString::number( volKnob->value() ) );

	for( int i = 0; i < m_num_oscillators; ++i )
	{
		QString is = QString::number( i );
		to_de.setAttribute( "vol" + is, QString::number(
						m_osc[i].volKnob->value() ) );
		to_de.setAttribute( "pan" + is, QString::number(
						m_osc[i].panKnob->value() ) );
		to_de.setAttribute( "harmonic" + is, QString::number(
						m_osc[i].harmonic ) );
		to_de.setAttribute( "detune" + is, QString::number(
						m_osc[i].detuneKnob->value() ) );
		to_de.setAttribute( "wavetype" + is, QString::number(
							m_osc[i].waveShape ) );
	}
	
	

	_parent.appendChild( to_de );


}




void organicInstrument::loadSettings( const QDomElement & _this )
{
//	m_num_oscillators =  _this.attribute( "num_osc" ).
	//							toInt();

	for( int i = 0; i < m_num_oscillators; ++i )
	{
		QString is = QString::number( i );
		m_osc[i].volKnob->setValue( _this.attribute( "vol" + is ).
								toFloat() );
		m_osc[i].detuneKnob->setValue( _this.attribute( "detune" + is ).
								toFloat() );
		m_osc[i].panKnob->setValue( _this.attribute( "pan" + is ).
								toFloat() );
		m_osc[i].oscKnob->setValue( _this.attribute( "wavetype"+is ).
								toInt() );								
	}
	
	volKnob->setValue( _this.attribute( "vol" ).
								toFloat() );
	fx1Knob->setValue( _this.attribute( "foldback" ).
								toFloat() );
								
	oscButtonChanged();	
}


QString organicInstrument::nodeName( void ) const
{
	return( organic_plugin_descriptor.name );
}




void organicInstrument::playNote( notePlayHandle * _n )
{
	if( _n->totalFramesPlayed() == 0 )
	{
		float freq = getChannelTrack()->frequency( _n );

		oscillator * oscs_l[m_num_oscillators];
		oscillator * oscs_r[m_num_oscillators];

		for( Sint8 i = m_num_oscillators-1; i >= 0; --i )
		{
			
			// volume
			float volume = m_osc[i].volKnob->value() / 100.0 / m_num_oscillators ;
			float volume_l = volume * ( m_osc[i].panKnob->value() +
						PANNING_RIGHT ) / 100.0f;
			float volume_r = volume * ( PANNING_RIGHT -
						m_osc[i].panKnob->value() ) /
									100.0f;

			// detuning
			float osc_detuning_l = +m_osc[i].detuneKnob->value() / 10.0f;
			float osc_detuning_r = -m_osc[i].detuneKnob->value() / 10.0f;
			
			// frequency
			float freq_l = freq * m_osc[i].harmonic + osc_detuning_l;
			float freq_r = freq * m_osc[i].harmonic + osc_detuning_r;
			
			// randomize the phaseOffset [0,360]
			int phase_l = (int) (rand() * 360.0);
			int phase_r = (int) (rand() * 360.0);
			

			
			// Nyquist boundary check
			if (freq > (eng()->getMixer()->sampleRate() >> 2))
			{
				volume = 0;
			}
			
			// initialise ocillators
			
			if (i == (m_num_oscillators-1)) {
				// create left oscillator
				oscs_l[i] = oscillator::createOsc(
						m_osc[i].waveShape,
						oscillator::MIX,
						freq_l,
						phase_l,
								volume_l,
					eng()->getMixer()->sampleRate() );
				// create right oscillator
				oscs_r[i] = oscillator::createOsc(
						m_osc[i].waveShape,
						oscillator::MIX,
						freq_r,
						phase_r,
								volume_r,
					eng()->getMixer()->sampleRate() );	
					
			} else {
				// create left oscillator
				oscs_l[i] = oscillator::createOsc(
						m_osc[i].waveShape,
						oscillator::MIX,
						freq_l,
						phase_l,
								volume_l,
					eng()->getMixer()->sampleRate(),
					oscs_l[i + 1] );
				// create right oscillator
				oscs_r[i] = oscillator::createOsc(
						m_osc[i].waveShape,
						oscillator::MIX,
						freq_r,
						phase_r,
								volume_r,
					eng()->getMixer()->sampleRate(),
					oscs_r[i + 1] );					
				
			}
			
				
		}

		_n->m_pluginData = new oscPtr;
		static_cast<oscPtr *>( _n->m_pluginData )->oscLeft = oscs_l[0];
		static_cast< oscPtr *>( _n->m_pluginData )->oscRight =
								oscs_r[0];
	}

	oscillator * osc_l = static_cast<oscPtr *>( _n->m_pluginData )->oscLeft;
	oscillator * osc_r = static_cast<oscPtr *>( _n->m_pluginData
								)->oscRight;

	const fpab_t frames = eng()->getMixer()->framesPerAudioBuffer();
	sampleFrame * buf = bufferAllocator::alloc<sampleFrame>( frames );
	
	osc_l->update( buf, frames, 0 );
	osc_r->update( buf, frames, 1 );


	// -- fx section --
	
	// fxKnob is [0;1]
	float t =  fx1Knob->value();
	
	for (int i=0 ; i < frames ; i++)
	{
		buf[i][0] = waveshape( buf[i][0], t ) * volKnob->value() / 100.0;	
		buf[i][1] = waveshape( buf[i][1], t ) * volKnob->value() / 100.0;	
	}
	
	// -- --

	getChannelTrack()->processAudioBuffer( buf, frames, _n );

	bufferAllocator::free( buf );
}




void organicInstrument::deleteNotePluginData( notePlayHandle * _n )
{
	if( _n->m_pluginData == NULL )
	{
		return;
	}
	delete static_cast<oscillator *>( static_cast<oscPtr *>(
						_n->m_pluginData )->oscLeft );
	delete static_cast<oscillator *>( static_cast<oscPtr *>(
						_n->m_pluginData )->oscRight );
	delete static_cast<oscPtr *>( _n->m_pluginData );
}

float organicInstrument::foldback(float in, float threshold)
{
  if (in>threshold || in<-threshold)
  {
    in= fabs(fabs(fmod(in - threshold, threshold*4)) - threshold*2) - threshold;
  }
  return in;
}

float organicInstrument::distort(float in, float dist)
{
		in *= dist;
		if (in >= 1.0f) {
			return 1.0f;
		} else {
			return in;
		}
	
}

float organicInstrument::saturate(float x, float t)
{
    if(fabs(x)<t)
        return x;
    else
    {
        if(x > 0.f)
            return t + (1.f-t)*tanh((x-t)/(1-t));
        else
            return -(t + (1.f-t)*tanh((-x-t)/(1-t)));
	}
}

float organicInstrument::waveshape(float in, float amount)
{
	float k = 2.0f*amount/(1.0f-amount);

	return (1.0f + k) *
		in / (1.0f + k * fabs( in ));	
}

void organicInstrument::oscButtonChanged( )
{
	
	for (int i = 0; i < m_num_oscillators; i++)
	{
		float value = m_osc[i].oscKnob->value();
		
		if ( value <= 0.5 ) {
				m_osc[i].waveShape = oscillator::SIN_WAVE;
				continue;
		}
		
		if ( value <= 1.5 ) {
				m_osc[i].waveShape = oscillator::SAW_WAVE;
				continue;
		}

		if ( value <= 2.5 ) {
				m_osc[i].waveShape = oscillator::SQUARE_WAVE;
				continue;
		}
		
		if ( value <= 3.5 ) {
				m_osc[i].waveShape = oscillator::TRIANGLE_WAVE;
				continue;
		}
		
		if ( value <= 4.5 ) {
				m_osc[i].waveShape = oscillator::MOOG_SAW_WAVE;
				continue;
		}
		
		m_osc[i].waveShape = oscillator::EXP_WAVE;
		
	}
}


extern "C"
{

// neccessary for getting instance out of shared lib
plugin * lmms_plugin_main( void * _data )
{
	return( new organicInstrument( static_cast<channelTrack *>( _data ) ) );
}


}

/*
 * some notes & ideas for the future of this plugin:
 * 
 * - 32.692 Hz in the bass to 5919.85 Hz of treble in  a Hammond organ
 * => implement harmonic foldback
 * 
 * - randomize preset
 */

