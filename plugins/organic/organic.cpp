/*
 * organic.cpp - additive synthesizer for organ-like sounds
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */


#include "qt3support.h"

#ifdef QT4

#include <QtGui/QPainter>
#include <Qt/QtXml>
#include <QtGui/QDropEvent>

#else

#include <qpainter.h>
#include <qdom.h>
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
#include "instrument_track.h"
#include "note_play_handle.h"
#include "templates.h"
#include "buffer_allocator.h"
#include "knob.h"
#include "pixmap_button.h"
#include "tooltip.h"
#include "oscillator.h"

#undef SINGLE_SOURCE_COMPILE
#include "embed.cpp"
#include "volume_knob.h"


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
	new QPixmap( PLUGIN_NAME::getIconPixmap( "logo" ) )
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


organicInstrument::organicInstrument( instrumentTrack * _channel_track ) :
	instrument( _channel_track,
			&organic_plugin_descriptor ),
	specialBgHandlingWidget( PLUGIN_NAME::getIconPixmap( "artwork" ) ),
	m_modulationAlgo( oscillator::MIX )
{


	m_num_oscillators = 8;

	m_osc = new oscillatorData[m_num_oscillators];
	
	m_osc[0].harmonic = log2f( 0.5f );	//	one octave below
	m_osc[1].harmonic = log2f( 0.75f );	//	a fifth below
	m_osc[2].harmonic = log2f( 1.0f );	// base freq
	m_osc[3].harmonic = log2f( 2.0f );	// first overtone
	m_osc[4].harmonic = log2f( 3.0f );	// second overtone
	m_osc[5].harmonic = log2f( 4.0f );	// .
	m_osc[6].harmonic = log2f( 5.0f );	// .
	m_osc[7].harmonic = log2f( 6.0f );	// .

	for (int i=0; i < m_num_oscillators; i++)
	{
		m_osc[i].waveShape = oscillator::SIN_WAVE;

		// setup volume-knob
		m_osc[i].oscKnob = new knob( knobGreen_17, this, tr(
					"Osc %1 waveform" ).arg( i+1 ), eng(),
							_channel_track );
		m_osc[i].oscKnob->move( 25+i*20, 90 );
		m_osc[i].oscKnob->setRange( 0.0f, 5.0f, 0.25f );
		m_osc[i].oscKnob->setInitValue( 0.0f );
		m_osc[i].oscKnob->setHintText( tr( "Osc %1 waveform:" ).arg(
							i+1 ) + " ", "%" );
										
		connect( m_osc[i].oscKnob, SIGNAL( valueChanged( float ) ),
			this, SLOT (oscButtonChanged( void ) ) 
		);
										
		// setup volume-knob
		m_osc[i].volKnob = new volumeKnob( knobGreen_17, this, tr(
					"Osc %1 volume" ).arg( i+1 ), eng(),
							_channel_track );
		m_osc[i].volKnob->setData( i );
		m_osc[i].volKnob->move( 25+i*20, 110 );
		m_osc[i].volKnob->setRange( 0, 100, 1.0f );
		m_osc[i].volKnob->setInitValue( 100 );
		m_osc[i].volKnob->setHintText( tr( "Osc %1 volume:" ).arg(
							i+1 ) + " ", "%" );
							
		// setup panning-knob
		m_osc[i].panKnob = new knob( knobGreen_17, this,
				tr( "Osc %1 panning" ).arg( i + 1 ), eng(),
							_channel_track );
		m_osc[i].panKnob->setData( i );
		m_osc[i].panKnob->move( 25+i*20, 130 );
		m_osc[i].panKnob->setRange( PANNING_LEFT, PANNING_RIGHT, 1.0f );
		m_osc[i].panKnob->setInitValue( DEFAULT_PANNING );
		m_osc[i].panKnob->setHintText( tr("Osc %1 panning:").arg( i+1 )
						+ " ", "" );
							
		// setup knob for left fine-detuning
		m_osc[i].detuneKnob = new knob( knobGreen_17, this,
				tr( "Osc %1 fine detuning left" ).arg( i+1 ),
							eng(), _channel_track );
		m_osc[i].detuneKnob->setData( i );
		m_osc[i].detuneKnob->move( 25+i*20, 150 );
		m_osc[i].detuneKnob->setRange( -100.0f, 100.0f, 1.0f );
		m_osc[i].detuneKnob->setInitValue( 0.0f );
		m_osc[i].detuneKnob->setHintText( tr( "Osc %1 fine detuning "
							"left:" ).arg( i + 1 )
							+ " ", " " +
							tr( "cents" ) );

		connect( m_osc[i].volKnob,
				SIGNAL( valueChanged( const QVariant & ) ),
			this, SLOT( updateVolume( const QVariant & ) ) );
		connect( m_osc[i].panKnob,
				SIGNAL( valueChanged( const QVariant & ) ),
			this, SLOT( updateVolume( const QVariant & ) ) );
		updateVolume( i );

		connect( m_osc[i].detuneKnob,
				SIGNAL ( valueChanged( const QVariant & ) ),
			this, SLOT( updateDetuning( const QVariant & ) ) );
		updateDetuning( i );

	}

	connect( eng()->getMixer(), SIGNAL( sampleRateChanged() ),
			this, SLOT( updateAllDetuning() ) );

		// setup knob for FX1
		fx1Knob = new knob( knobGreen_17, this,
				tr( "FX1" ), eng(), _channel_track );
		fx1Knob->move( 20, 200 );
		fx1Knob->setRange( 0.0f, 0.99f, 0.01f );
		fx1Knob->setInitValue( 0.0f);
		
		// setup volume-knob
		volKnob = new knob( knobGreen_17, this, tr(
					"Osc %1 volume" ).arg( 1 ), eng(),
							_channel_track );
		volKnob->move( 50, 200 );
		volKnob->setRange( 0, 200, 1.0f );
		volKnob->setInitValue( 100 );
		volKnob->setHintText( tr( "Osc %1 volume:" ).arg(
							1 ) + " ", "%" );
							
		// randomise
		m_randBtn = new pixmapButton( this, tr( "Randomise" ), eng(),
							_channel_track );
		m_randBtn->move( 100, 200 );
		m_randBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"randomise_pressed" ) );
		m_randBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"randomise" ) );
		//m_randBtn->setMask( QBitmap( PLUGIN_NAME::getIconPixmap( "btn_mask" ).
		//				createHeuristicMask() ) );

		connect( m_randBtn, SIGNAL ( clicked() ),
			this, SLOT( randomiseSettings() ) );


	if( s_artwork == NULL )
	{
		s_artwork = new QPixmap( PLUGIN_NAME::getIconPixmap(
								"artwork" ) );
	}



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



organicInstrument::~organicInstrument()
{
}




void organicInstrument::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setAttribute( "num_osc", QString::number( m_num_oscillators ) );
	fx1Knob->saveSettings( _doc, _this, "foldback" );
	volKnob->saveSettings( _doc, _this, "vol" );

	for( int i = 0; i < m_num_oscillators; ++i )
	{
		QString is = QString::number( i );
		m_osc[i].volKnob->saveSettings( _doc, _this, "vol" + is );
		m_osc[i].panKnob->saveSettings( _doc, _this, "pan" + is );
		_this.setAttribute( "harmonic" + is, QString::number(
					powf( 2.0f, m_osc[i].harmonic ) ) );
		m_osc[i].detuneKnob->saveSettings( _doc, _this, "detune" + is );
		m_osc[i].oscKnob->saveSettings( _doc, _this, "wavetype" + is );
	}
}




void organicInstrument::loadSettings( const QDomElement & _this )
{
//	m_num_oscillators =  _this.attribute( "num_osc" ).
	//							toInt();

	for( int i = 0; i < m_num_oscillators; ++i )
	{
		QString is = QString::number( i );
		m_osc[i].volKnob->loadSettings( _this, "vol" + is );
		m_osc[i].detuneKnob->loadSettings( _this, "detune" + is );
		m_osc[i].panKnob->loadSettings( _this, "pan" + is );
		m_osc[i].oscKnob->loadSettings( _this, "wavetype" + is );
	}
	
	volKnob->loadSettings( _this, "vol" );
	fx1Knob->loadSettings( _this, "foldback" );
								
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
		oscillator * oscs_l[m_num_oscillators];
		oscillator * oscs_r[m_num_oscillators];

		for( Sint8 i = m_num_oscillators-1; i >= 0; --i )
		{
			
			// randomize the phaseOffset [0,1)
			m_osc[i].phaseOffsetLeft = rand()
							/ ( RAND_MAX + 1.0f );
			m_osc[i].phaseOffsetRight = rand()
							/ ( RAND_MAX + 1.0f );
			

			
			// initialise ocillators
			
			if (i == (m_num_oscillators-1)) {
				// create left oscillator
				oscs_l[i] = new oscillator(
						&m_osc[i].waveShape,
						&m_modulationAlgo,
						&_n->m_frequency,
						&m_osc[i].detuningLeft,
						&m_osc[i].phaseOffsetLeft,
						&m_osc[i].volumeLeft );
				// create right oscillator
				oscs_r[i] = new oscillator(
						&m_osc[i].waveShape,
						&m_modulationAlgo,
						&_n->m_frequency,
						&m_osc[i].detuningRight,
						&m_osc[i].phaseOffsetRight,
						&m_osc[i].volumeRight );	
					
			} else {
				// create left oscillator
				oscs_l[i] = new oscillator(
						&m_osc[i].waveShape,
						&m_modulationAlgo,
						&_n->m_frequency,
						&m_osc[i].detuningLeft,
						&m_osc[i].phaseOffsetLeft,
						&m_osc[i].volumeLeft,
					oscs_l[i + 1] );
				// create right oscillator
				oscs_r[i] = new oscillator(
						&m_osc[i].waveShape,
						&m_modulationAlgo,
						&_n->m_frequency,
						&m_osc[i].detuningRight,
						&m_osc[i].phaseOffsetRight,
						&m_osc[i].volumeRight,
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
		buf[i][0] = waveshape( buf[i][0], t ) * volKnob->value()
								/ 100.0f;
		buf[i][1] = waveshape( buf[i][1], t ) * volKnob->value()
								/ 100.0f;
	}
	
	// -- --

	getInstrumentTrack()->processAudioBuffer( buf, frames, _n );

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

/*float inline organicInstrument::foldback(float in, float threshold)
{
  if (in>threshold || in<-threshold)
  {
    in= fabs(fabs(fmod(in - threshold, threshold*4)) - threshold*2) - threshold;
  }
  return in;
}
*/

float inline organicInstrument::waveshape(float in, float amount)
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

void organicInstrument::randomiseSettings()
{
	
	for (int i=0; i < m_num_oscillators; i++)
	{
		m_osc[i].volKnob->setValue( 
			intRand(0,100)
		 );
	
		m_osc[i].detuneKnob->setValue( 
			intRand(-5, 5)
		 );
	
		m_osc[i].panKnob->setValue( 
			//(int)gaussRand(PANNING_LEFT, PANNING_RIGHT,1,0)
			0
		  );
		  
		m_osc[i].oscKnob->setValue(  
			intRand(0, 5)
		);	
	}
	
}




void organicInstrument::updateVolume( const QVariant & _data )
{
	const int _i = _data.toInt();
	m_osc[_i].volumeLeft =
		( 1.0f - m_osc[_i].panKnob->value() / (float)PANNING_RIGHT )
		* m_osc[_i].volKnob->value() / m_num_oscillators / 100.0f;
	m_osc[_i].volumeRight =
		( 1.0f + m_osc[_i].panKnob->value() / (float)PANNING_RIGHT )
		* m_osc[_i].volKnob->value() / m_num_oscillators / 100.0f;
}




void organicInstrument::updateDetuning( const QVariant & _data )
{
	const int _i = _data.toInt();
	m_osc[_i].detuningLeft = powf( 2.0f, m_osc[_i].harmonic
			+ (float)m_osc[_i].detuneKnob->value() / 100.0f )
			/ static_cast<float>( eng()->getMixer()->sampleRate() );
	m_osc[_i].detuningRight = powf( 2.0f, m_osc[_i].harmonic
			- (float)m_osc[_i].detuneKnob->value() / 100.0f )
			/ static_cast<float>( eng()->getMixer()->sampleRate() );
}




void organicInstrument::updateAllDetuning( void )
{
	for( int i = 0; i < m_num_oscillators; ++i )
	{
		updateDetuning( i );
	}
}




int organicInstrument::intRand( int min, int max )
{
//	int randn = min+int((max-min)*rand()/(RAND_MAX + 1.0));	
//	cout << randn << endl;
	int randn = ( rand() % (max-min) ) + min ;
	return randn;
}




extern "C"
{

// neccessary for getting instance out of shared lib
plugin * lmms_plugin_main( void * _data )
{
	return( new organicInstrument( static_cast<instrumentTrack *>( _data ) ) );
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

