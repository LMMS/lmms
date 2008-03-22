/*
 * organic.cpp - additive synthesizer for organ-like sounds
 *
 * Copyright (c) 2006-2007 Andreas Brandmaier <andy/at/brandmaier/dot/de>
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


#include "organic.h"


#include <QtXml/QDomElement>


#include "engine.h"
#include "instrument_track.h"
#include "knob.h"
#include "note_play_handle.h"
#include "oscillator.h"
#include "pixmap_button.h"
#include "templates.h"
#include "tooltip.h"
#include "volume_knob.h"

#undef SINGLE_SOURCE_COMPILE
#include "embed.cpp"


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
	plugin::Instrument,
	new QPixmap( PLUGIN_NAME::getIconPixmap( "logo" ) ),
	NULL
} ;

}

QPixmap * organicInstrumentView::s_artwork = NULL;


/***********************************************************************
*
*	class OrganicInstrument
*
*	lmms - plugin 
*
***********************************************************************/


organicInstrument::organicInstrument( instrumentTrack * _channel_track ) :
	instrument( _channel_track, &organic_plugin_descriptor ),
	m_modulationAlgo( oscillator::SignalMix ),
	m_fx1Knob( 0.0f, 0.0f, 0.99f, 0.01f , this ),
	m_volKnob( 100.0f, 0.0f, 200.0f, 1.0f, this )
{
	m_numOscillators = 8;

	m_osc = new oscillatorObject*[ m_numOscillators ];
	for (int i=0; i < m_numOscillators; i++)
	{
		m_osc[i] = new oscillatorObject( this, _channel_track );
		m_osc[i]->m_numOscillators = m_numOscillators;

		// Connect events 
		connect( &m_osc[i]->m_oscKnob, SIGNAL( dataChanged() ),
				m_osc[i], SLOT ( oscButtonChanged() ) );
		connect( &m_osc[i]->m_volKnob, SIGNAL( dataChanged() ),
				m_osc[i], SLOT( updateVolume() ) );
		connect( &m_osc[i]->m_panKnob, SIGNAL( dataChanged() ),
				m_osc[i], SLOT( updateVolume() ) );
		connect( &m_osc[i]->m_detuneKnob, SIGNAL( dataChanged() ),
				m_osc[i], SLOT( updateDetuning() ) );
		
		m_osc[i]->updateVolume();

	}

	m_osc[0]->m_harmonic = log2f( 0.5f );	// one octave below
	m_osc[1]->m_harmonic = log2f( 0.75f );	// a fifth below
	m_osc[2]->m_harmonic = log2f( 1.0f );	// base freq
	m_osc[3]->m_harmonic = log2f( 2.0f );	// first overtone
	m_osc[4]->m_harmonic = log2f( 3.0f );	// second overtone
	m_osc[5]->m_harmonic = log2f( 4.0f );	// .
	m_osc[6]->m_harmonic = log2f( 5.0f );	// .
	m_osc[7]->m_harmonic = log2f( 6.0f );	// .

	for (int i=0; i < m_numOscillators; i++) {
		m_osc[i]->updateVolume();
		m_osc[i]->updateDetuning();
	}
	

	connect( engine::getMixer(), SIGNAL( sampleRateChanged() ),
					this, SLOT( updateAllDetuning() ) );

}




organicInstrument::~organicInstrument()
{
	delete[] m_osc;
}




void organicInstrument::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setAttribute( "num_osc", QString::number( m_numOscillators ) );
	m_fx1Knob.saveSettings( _doc, _this, "foldback" );
	m_volKnob.saveSettings( _doc, _this, "vol" );

	for( int i = 0; i < m_numOscillators; ++i )
	{
		QString is = QString::number( i );
		m_osc[i]->m_volKnob.saveSettings( _doc, _this, "vol" + is );
		m_osc[i]->m_panKnob.saveSettings( _doc, _this, "pan" + is );
		_this.setAttribute( "harmonic" + is, QString::number(
					powf( 2.0f, m_osc[i]->m_harmonic ) ) );
		m_osc[i]->m_detuneKnob.saveSettings( _doc, _this, "detune"
									+ is );
		m_osc[i]->m_oscKnob.saveSettings( _doc, _this, "wavetype"
									+ is );
	}
}




void organicInstrument::loadSettings( const QDomElement & _this )
{
//	m_numOscillators =  _this.attribute( "num_osc" ).
	//							toInt();

	for( int i = 0; i < m_numOscillators; ++i )
	{
		QString is = QString::number( i );
		m_osc[i]->m_volKnob.loadSettings( _this, "vol" + is );
		m_osc[i]->m_detuneKnob.loadSettings( _this, "detune" + is );
		m_osc[i]->m_panKnob.loadSettings( _this, "pan" + is );
		m_osc[i]->m_oscKnob.loadSettings( _this, "wavetype" + is );
	}
	
	m_volKnob.loadSettings( _this, "vol" );
	m_fx1Knob.loadSettings( _this, "foldback" );
}


QString organicInstrument::nodeName( void ) const
{
	return( organic_plugin_descriptor.name );
}




void organicInstrument::playNote( notePlayHandle * _n, bool )
{
	if( _n->totalFramesPlayed() == 0 || _n->m_pluginData == NULL )
	{
		oscillator * oscs_l[m_numOscillators];
		oscillator * oscs_r[m_numOscillators];

		for( Sint8 i = m_numOscillators - 1; i >= 0; --i )
		{
			
			// randomize the phaseOffset [0,1)
			m_osc[i]->m_phaseOffsetLeft = rand()
							/ ( RAND_MAX + 1.0f );
			m_osc[i]->m_phaseOffsetRight = rand()
							/ ( RAND_MAX + 1.0f );
			

			
			// initialise ocillators
			
			if( i == m_numOscillators - 1 )
			{
				// create left oscillator
				oscs_l[i] = new oscillator(
						m_osc[i]->m_waveShape,
						m_modulationAlgo,
						_n->frequency(),
						m_osc[i]->m_detuningLeft,
						m_osc[i]->m_phaseOffsetLeft,
						m_osc[i]->m_volumeLeft );
				// create right oscillator
				oscs_r[i] = new oscillator(
						m_osc[i]->m_waveShape,
						m_modulationAlgo,
						_n->frequency(),
						m_osc[i]->m_detuningRight,
						m_osc[i]->m_phaseOffsetRight,
						m_osc[i]->m_volumeRight );
			}
			else
			{
				// create left oscillator
				oscs_l[i] = new oscillator(
						m_osc[i]->m_waveShape,
						m_modulationAlgo,
						_n->frequency(),
						m_osc[i]->m_detuningLeft,
						m_osc[i]->m_phaseOffsetLeft,
						m_osc[i]->m_volumeLeft,
						oscs_l[i + 1] );
				// create right oscillator
				oscs_r[i] = new oscillator(
						m_osc[i]->m_waveShape,
						m_modulationAlgo,
						_n->frequency(),
						m_osc[i]->m_detuningRight,
						m_osc[i]->m_phaseOffsetRight,
						m_osc[i]->m_volumeRight,
						oscs_r[i + 1] );
			}
			
				
		}

		_n->m_pluginData = new oscPtr;
		static_cast<oscPtr *>( _n->m_pluginData )->oscLeft = oscs_l[0];
		static_cast<oscPtr *>( _n->m_pluginData )->oscRight = oscs_r[0];
	}

	oscillator * osc_l = static_cast<oscPtr *>( _n->m_pluginData )->oscLeft;
	oscillator * osc_r = static_cast<oscPtr *>( _n->m_pluginData
								)->oscRight;

	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	sampleFrame * buf = new sampleFrame[frames];
	
	osc_l->update( buf, frames, 0 );
	osc_r->update( buf, frames, 1 );


	// -- fx section --
	
	// fxKnob is [0;1]
	float t =  m_fx1Knob.value();
	
	for (int i=0 ; i < frames ; i++)
	{
		buf[i][0] = waveshape( buf[i][0], t ) * m_volKnob.value()
								/ 100.0f;
		buf[i][1] = waveshape( buf[i][1], t ) * m_volKnob.value()
								/ 100.0f;
	}
	
	// -- --

	getInstrumentTrack()->processAudioBuffer( buf, frames, _n );

	delete[] buf;
}




void organicInstrument::deleteNotePluginData( notePlayHandle * _n )
{
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
	float k = 2.0f * amount / ( 1.0f - amount );

	return( ( 1.0f + k ) * in / ( 1.0f + k * fabs( in ) ) );
}




void organicInstrument::randomiseSettings( void )
{

	for( int i = 0; i < m_numOscillators; i++ )
	{
		m_osc[i]->m_volKnob.setValue( intRand( 0, 100 ) );

		m_osc[i]->m_detuneKnob.setValue( intRand( -5, 5 ) );

		m_osc[i]->m_panKnob.setValue(
			//(int)gaussRand(PANNING_LEFT, PANNING_RIGHT,1,0)
			0 );

		m_osc[i]->m_oscKnob.setValue( intRand( 0, 5 ) );
	}

}




void organicInstrument::updateAllDetuning( void )
{
	for( int i = 0; i < m_numOscillators; ++i )
	{
		m_osc[i]->updateDetuning();
	}
}




int organicInstrument::intRand( int min, int max )
{
//	int randn = min+int((max-min)*rand()/(RAND_MAX + 1.0));	
//	cout << randn << endl;
	int randn = ( rand() % (max - min) ) + min;
	return( randn );
}


pluginView * organicInstrument::instantiateView( QWidget * _parent )
{
	return( new organicInstrumentView( this, _parent ) );
}


organicInstrumentView::organicInstrumentView( instrument * _instrument,
							QWidget * _parent ) :
	instrumentView( _instrument, _parent )
{
	organicInstrument * oi = castModel<organicInstrument>();

	setAutoFillBackground( TRUE );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap(
								"artwork" ) );
	setPalette( pal );

	// setup knob for FX1
	m_fx1Knob = new knob( knobGreen_17, this, tr( "FX1" ) );
	m_fx1Knob->move( 20, 200 );

	// setup volume-knob
	m_volKnob = new knob( knobGreen_17, this, tr( "Osc %1 volume" ).arg(
							1 ) );
	m_volKnob->move( 50, 200 );
	m_volKnob->setHintText( tr( "Osc %1 volume:" ).arg( 1 ) + " ", "%" );

	// randomise
	m_randBtn = new pixmapButton( this, tr( "Randomise" ) );
	m_randBtn->move( 100, 200 );
	m_randBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"randomise_pressed" ) );
	m_randBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
								"randomise" ) );
	//m_randBtn->setMask( QBitmap( PLUGIN_NAME::getIconPixmap( "btn_mask" ).
	//				createHeuristicMask() ) );

	connect( m_randBtn, SIGNAL ( clicked() ),
					oi, SLOT( randomiseSettings() ) );


	if( s_artwork == NULL )
	{
		s_artwork = new QPixmap( PLUGIN_NAME::getIconPixmap(
								"artwork" ) );
	}

}


organicInstrumentView::~organicInstrumentView()
{
	delete[] m_oscKnobs;
}


void organicInstrumentView::modelChanged( void )
{
	organicInstrument * oi = castModel<organicInstrument>();

	m_numOscillators = oi->m_numOscillators;
	
	m_fx1Knob->setModel( &oi->m_fx1Knob );
	m_volKnob->setModel( &oi->m_volKnob );

	// TODO: Delete existing oscKnobs if they exist
	
	m_oscKnobs = new oscillatorKnobs[ m_numOscillators ];

	// Create knobs, now that we know how many to make
	for( int i = 0; i < m_numOscillators; ++i )
	{
		// setup waveform-knob
		knob * oscKnob = new knob( knobGreen_17, this, tr(
					"Osc %1 waveform" ).arg( i + 1 ) );
		oscKnob->move( 25 + i * 20, 90 );
		oscKnob->setHintText( tr( "Osc %1 waveform:" ).arg(
					i + 1 ) + " ", "%" );
										
		// setup volume-knob
		volumeKnob * volKnob = new volumeKnob( knobGreen_17, this, tr(
						"Osc %1 volume" ).arg( i + 1 ) );
		volKnob->move( 25 + i * 20, 110 );
		volKnob->setHintText( tr( "Osc %1 volume:" ).arg(
							i + 1 ) + " ", "%" );
							
		// setup panning-knob
		knob * panKnob = new knob( knobGreen_17, this,
					tr( "Osc %1 panning" ).arg( i + 1 ) );
		panKnob->move( 25 + i  * 20, 130 );
		panKnob->setHintText( tr("Osc %1 panning:").arg(
							i + 1 ) + " ", "" );
							
		// setup knob for left fine-detuning
		knob * detuneKnob = new knob( knobGreen_17, this,
				tr( "Osc %1 fine detuning left" ).arg( i + 1 ) );
		detuneKnob->move( 25 + i * 20, 150 );
		detuneKnob->setHintText( tr( "Osc %1 fine detuning "
							"left:" ).arg( i + 1 )
							+ " ", " " +
							tr( "cents" ) );

		m_oscKnobs[i] = oscillatorKnobs( volKnob, oscKnob, panKnob, detuneKnob );

		// Attach to models
		m_oscKnobs[i].m_volKnob->setModel(
					&oi->m_osc[i]->m_volKnob );
		m_oscKnobs[i].m_oscKnob->setModel(
					&oi->m_osc[i]->m_oscKnob );
		m_oscKnobs[i].m_panKnob->setModel(
					&oi->m_osc[i]->m_panKnob );
		m_oscKnobs[i].m_detuneKnob->setModel(
					&oi->m_osc[i]->m_detuneKnob );


		/*connect( m_oscKnobs[i].m_userWaveButton,
						SIGNAL( doubleClicked() ),
				t->m_osc[i], SLOT( oscUserDefWaveDblClick() ) );
				*/
	}
}


oscillatorObject::oscillatorObject( model * _parent, track * _track ) :
	model( _parent ),
	m_waveShape( oscillator::SineWave, 0, oscillator::NumWaveShapes-1, 1, this ),
	m_oscKnob( 0.0f, 0.0f, 5.0f, 1.0f, this ),
	m_volKnob( 100.0f, 0.0f, 100.0f, 1.0f, this ),
	m_panKnob( DEFAULT_PANNING, PANNING_LEFT, PANNING_RIGHT, 1.0f, this ),
	m_detuneKnob( 0.0f, -100.0f, 100.0f, 1.0f, this ) 
{
}




oscillatorObject::~oscillatorObject()
{
}




void oscillatorObject::oscButtonChanged( void )
{

	static oscillator::WaveShapes shapes[] =
	{
		oscillator::SineWave,
		oscillator::SawWave,
		oscillator::SquareWave,
		oscillator::TriangleWave,
		oscillator::MoogSawWave,
		oscillator::ExponentialWave
	} ;

	m_waveShape.setValue( shapes[(int)roundf( m_oscKnob.value() )] );

}




void oscillatorObject::updateVolume( void )
{
	m_volumeLeft = ( 1.0f - m_panKnob.value() / (float)PANNING_RIGHT )
			* m_volKnob.value() / m_numOscillators / 100.0f;
	m_volumeRight = ( 1.0f + m_panKnob.value() / (float)PANNING_RIGHT )
			* m_volKnob.value() / m_numOscillators / 100.0f;
}




void oscillatorObject::updateDetuning( void )
{
	m_detuningLeft = powf( 2.0f, m_harmonic
				+ (float)m_detuneKnob.value() / 100.0f )
					/ engine::getMixer()->sampleRate();
	m_detuningRight = powf( 2.0f, m_harmonic
				- (float)m_detuneKnob.value() / 100.0f )
					/ engine::getMixer()->sampleRate();
}




extern "C"
{

// neccessary for getting instance out of shared lib
plugin * lmms_plugin_main( model *, void * _data )
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
 m_osc[i].m_oscKnob->setInitValue( 0.0f );
 * - randomize preset 
 */



#include "organic.moc"
