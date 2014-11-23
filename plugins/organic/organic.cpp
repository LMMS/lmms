/*
 * organic.cpp - additive synthesizer for organ-like sounds
 *
 * Copyright (c) 2006-2008 Andreas Brandmaier <andy/at/brandmaier/dot/de>
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


#include "organic.h"


#include <QtXml/QDomElement>
#include <QtGui/QPainter>


#include "engine.h"
#include "InstrumentTrack.h"
#include "knob.h"
#include "NotePlayHandle.h"
#include "Oscillator.h"
#include "pixmap_button.h"
#include "templates.h"
#include "tooltip.h"

#include "embed.cpp"




extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT organic_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"Organic",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"Additive Synthesizer for organ-like sounds" ),
	"Andreas Brandmaier <andreas/at/brandmaier.de>",
	0x0100,
	Plugin::Instrument,
	new PluginPixmapLoader( "logo" ),
	NULL,
	NULL
} ;

}

QPixmap * organicInstrumentView::s_artwork = NULL;
float * organicInstrument::s_harmonics = NULL;

/***********************************************************************
*
*	class OrganicInstrument
*
*	lmms - plugin 
*
***********************************************************************/


organicInstrument::organicInstrument( InstrumentTrack * _instrument_track ) :
	Instrument( _instrument_track, &organic_plugin_descriptor ),
	m_modulationAlgo( Oscillator::SignalMix, Oscillator::SignalMix, Oscillator::SignalMix),
	m_fx1Model( 0.0f, 0.0f, 0.99f, 0.01f , this, tr( "Distortion" ) ),
	m_volModel( 100.0f, 0.0f, 200.0f, 1.0f, this, tr( "Volume" ) )
{
	m_numOscillators = 8;

	m_osc = new OscillatorObject*[ m_numOscillators ];
	for (int i=0; i < m_numOscillators; i++)
	{
		m_osc[i] = new OscillatorObject( this, i );
		m_osc[i]->m_numOscillators = m_numOscillators;

		// Connect events 
		connect( &m_osc[i]->m_oscModel, SIGNAL( dataChanged() ),
				m_osc[i], SLOT ( oscButtonChanged() ) );
		connect( &m_osc[i]->m_harmModel, SIGNAL( dataChanged() ),
				m_osc[i], SLOT( updateDetuning() ) );
		connect( &m_osc[i]->m_volModel, SIGNAL( dataChanged() ),
				m_osc[i], SLOT( updateVolume() ) );
		connect( &m_osc[i]->m_panModel, SIGNAL( dataChanged() ),
				m_osc[i], SLOT( updateVolume() ) );
		connect( &m_osc[i]->m_detuneModel, SIGNAL( dataChanged() ),
				m_osc[i], SLOT( updateDetuning() ) );

		m_osc[i]->updateVolume();

	}

/*	m_osc[0]->m_harmonic = log2f( 0.5f );	// one octave below
	m_osc[1]->m_harmonic = log2f( 0.75f );	// a fifth below
	m_osc[2]->m_harmonic = log2f( 1.0f );	// base freq
	m_osc[3]->m_harmonic = log2f( 2.0f );	// first overtone
	m_osc[4]->m_harmonic = log2f( 3.0f );	// second overtone
	m_osc[5]->m_harmonic = log2f( 4.0f );	// .
	m_osc[6]->m_harmonic = log2f( 5.0f );	// .
	m_osc[7]->m_harmonic = log2f( 6.0f );	// .*/
	
	if( s_harmonics == NULL )
	{
		s_harmonics = new float[ NUM_HARMONICS ];
		s_harmonics[0] = log2f( 0.5f );
		s_harmonics[1] = log2f( 0.75f );
		s_harmonics[2] = log2f( 1.0f );
		s_harmonics[3] = log2f( 2.0f );
		s_harmonics[4] = log2f( 3.0f );
		s_harmonics[5] = log2f( 4.0f );
		s_harmonics[6] = log2f( 5.0f );
		s_harmonics[7] = log2f( 6.0f );
		s_harmonics[8] = log2f( 7.0f );
		s_harmonics[9] = log2f( 8.0f );
		s_harmonics[10] = log2f( 9.0f );
		s_harmonics[11] = log2f( 10.0f );
		s_harmonics[12] = log2f( 11.0f );
		s_harmonics[13] = log2f( 12.0f );
		s_harmonics[14] = log2f( 13.0f );
		s_harmonics[15] = log2f( 14.0f );
		s_harmonics[16] = log2f( 15.0f );
		s_harmonics[17] = log2f( 16.0f );
	}

	for (int i=0; i < m_numOscillators; i++) {
		m_osc[i]->updateVolume();
		m_osc[i]->updateDetuning();
	}
	

	connect( engine::mixer(), SIGNAL( sampleRateChanged() ),
					this, SLOT( updateAllDetuning() ) );	
}




organicInstrument::~organicInstrument()
{
	delete[] m_osc;
}




void organicInstrument::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setAttribute( "num_osc", QString::number( m_numOscillators ) );
	m_fx1Model.saveSettings( _doc, _this, "foldback" );
	m_volModel.saveSettings( _doc, _this, "vol" );

	for( int i = 0; i < m_numOscillators; ++i )
	{
		QString is = QString::number( i );
		m_osc[i]->m_volModel.saveSettings( _doc, _this, "vol" + is );
		m_osc[i]->m_panModel.saveSettings( _doc, _this, "pan" + is );
		m_osc[i]->m_harmModel.saveSettings( _doc, _this, "newharmonic" + is );

		m_osc[i]->m_detuneModel.saveSettings( _doc, _this, "newdetune"
									+ is );
		m_osc[i]->m_oscModel.saveSettings( _doc, _this, "wavetype"
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
		m_osc[i]->m_volModel.loadSettings( _this, "vol" + is );
		if( _this.hasAttribute( "detune" + is ) )
		{
			m_osc[i]->m_detuneModel.setValue( _this.attribute( "detune" ).toInt() * 12 );			
		}
		else
		{
			m_osc[i]->m_detuneModel.loadSettings( _this, "newdetune" + is );
		}
		m_osc[i]->m_panModel.loadSettings( _this, "pan" + is );
		m_osc[i]->m_oscModel.loadSettings( _this, "wavetype" + is );
		
		if( _this.hasAttribute( "newharmonic" + is ) )
		{
			m_osc[i]->m_harmModel.loadSettings( _this, "newharmonic" + is );
		}
		else
		{
			m_osc[i]->m_harmModel.setValue( static_cast<float>( i ) );
		}
	}
	
	m_volModel.loadSettings( _this, "vol" );
	m_fx1Model.loadSettings( _this, "foldback" );
}


QString organicInstrument::nodeName() const
{
	return( organic_plugin_descriptor.name );
}




void organicInstrument::playNote( NotePlayHandle * _n,
						sampleFrame * _working_buffer )
{
	if( _n->totalFramesPlayed() == 0 || _n->m_pluginData == NULL )
	{
		Oscillator * oscs_l[m_numOscillators];
		Oscillator * oscs_r[m_numOscillators];

		for( int i = m_numOscillators - 1; i >= 0; --i )
		{
			
			m_osc[i]->m_phaseOffsetLeft = rand()
							/ ( RAND_MAX + 1.0f );
			m_osc[i]->m_phaseOffsetRight = rand()
							/ ( RAND_MAX + 1.0f );
			

			
			// initialise ocillators
			
			if( i == m_numOscillators - 1 )
			{
				// create left oscillator
				oscs_l[i] = new Oscillator(
						&m_osc[i]->m_waveShape,
						&m_modulationAlgo,
						_n->frequency(),
						m_osc[i]->m_detuningLeft,
						m_osc[i]->m_phaseOffsetLeft,
						m_osc[i]->m_volumeLeft );
				// create right oscillator
				oscs_r[i] = new Oscillator(
						&m_osc[i]->m_waveShape,
						&m_modulationAlgo,
						_n->frequency(),
						m_osc[i]->m_detuningRight,
						m_osc[i]->m_phaseOffsetRight,
						m_osc[i]->m_volumeRight );
			}
			else
			{
				// create left oscillator
				oscs_l[i] = new Oscillator(
						&m_osc[i]->m_waveShape,
						&m_modulationAlgo,
						_n->frequency(),
						m_osc[i]->m_detuningLeft,
						m_osc[i]->m_phaseOffsetLeft,
						m_osc[i]->m_volumeLeft,
						oscs_l[i + 1] );
				// create right oscillator
				oscs_r[i] = new Oscillator(
						&m_osc[i]->m_waveShape,
						&m_modulationAlgo,
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

	Oscillator * osc_l = static_cast<oscPtr *>( _n->m_pluginData )->oscLeft;
	Oscillator * osc_r = static_cast<oscPtr *>( _n->m_pluginData)->oscRight;

	const fpp_t frames = _n->framesLeftForCurrentPeriod();

	osc_l->update( _working_buffer, frames, 0 );
	osc_r->update( _working_buffer, frames, 1 );


	// -- fx section --
	
	// fxKnob is [0;1]
	float t =  m_fx1Model.value();
	
	for (int i=0 ; i < frames ; i++)
	{
		_working_buffer[i][0] = waveshape( _working_buffer[i][0], t ) *
						m_volModel.value() / 100.0f;
		_working_buffer[i][1] = waveshape( _working_buffer[i][1], t ) *
						m_volModel.value() / 100.0f;
	}
	
	// -- --

	instrumentTrack()->processAudioBuffer( _working_buffer, frames, _n );
}




void organicInstrument::deleteNotePluginData( NotePlayHandle * _n )
{
	delete static_cast<Oscillator *>( static_cast<oscPtr *>(
						_n->m_pluginData )->oscLeft );
	delete static_cast<Oscillator *>( static_cast<oscPtr *>(
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




void organicInstrument::randomiseSettings()
{

	for( int i = 0; i < m_numOscillators; i++ )
	{
		m_osc[i]->m_volModel.setValue( intRand( 0, 100 ) );

		m_osc[i]->m_detuneModel.setValue( intRand( -5, 5 ) );

		m_osc[i]->m_panModel.setValue( 0 );

		m_osc[i]->m_oscModel.setValue( intRand( 0, 5 ) );
	}

}




void organicInstrument::updateAllDetuning()
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


PluginView * organicInstrument::instantiateView( QWidget * _parent )
{
	return( new organicInstrumentView( this, _parent ) );
}




class organicKnob : public knob
{
public:
	organicKnob( QWidget * _parent ) :
		knob( knobStyled, _parent )
	{
		setFixedSize( 21, 21 );
	}
};




organicInstrumentView::organicInstrumentView( Instrument * _instrument,
							QWidget * _parent ) :
	InstrumentView( _instrument, _parent ),
	m_oscKnobs( NULL )
{
	organicInstrument * oi = castModel<organicInstrument>();

	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap(
								"artwork" ) );
	setPalette( pal );

	// setup knob for FX1
	m_fx1Knob = new organicKnob( this );
	m_fx1Knob->move( 15, 201 );
	m_fx1Knob->setFixedSize( 37, 47 );
	m_fx1Knob->setHintText( tr( "Distortion:" ) + " ", QString() );
	m_fx1Knob->setObjectName( "fx1Knob" );
	m_fx1Knob->setWhatsThis( tr( "The distortion knob adds distortion to the output of the instrument. " ) );

	// setup volume-knob
	m_volKnob = new organicKnob( this );
	m_volKnob->setVolumeKnob( true );
	m_volKnob->move( 60, 201 );
	m_volKnob->setFixedSize( 37, 47 );
	m_volKnob->setHintText( tr( "Volume:" ) + " ", "%" );
	m_volKnob->setObjectName( "volKnob" );
	m_volKnob->setWhatsThis( tr( "The volume knob controls the volume of the output of the instrument. "
									"It is cumulative with the instrument window's volume control. " ) );

	// randomise
	m_randBtn = new pixmapButton( this, tr( "Randomise" ) );
	m_randBtn->move( 148, 224 );
	m_randBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"randomise_pressed" ) );
	m_randBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
								"randomise" ) );
	m_randBtn->setWhatsThis( tr( "The randomize button randomizes all knobs except the harmonics,"
									"main volume and distortion knobs. ") );
	
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


void organicInstrumentView::modelChanged()
{
	organicInstrument * oi = castModel<organicInstrument>();
	
	const float y=91.0f;
	const float rowHeight = 26.0f;
	const float x=53.0f;
	const float colWidth = 24.0f; 

	m_numOscillators = oi->m_numOscillators;
	
	m_fx1Knob->setModel( &oi->m_fx1Model );
	m_volKnob->setModel( &oi->m_volModel );

	if( m_oscKnobs != NULL ) 
	{
		delete[] m_oscKnobs;
	}
	
	m_oscKnobs = new OscillatorKnobs[ m_numOscillators ];

	// Create knobs, now that we know how many to make
	for( int i = 0; i < m_numOscillators; ++i )
	{
		// setup harmonic knob
		knob * harmKnob = new organicKnob( this );
		harmKnob->move( x + i * colWidth, y - rowHeight );
		harmKnob->setObjectName( "harmKnob" );
		connect( &oi->m_osc[i]->m_harmModel, SIGNAL( dataChanged() ),
			this, SLOT( updateKnobHint() ) );
			
		// setup waveform-knob
		knob * oscKnob = new organicKnob( this );
		oscKnob->move( x + i * colWidth, y );
		connect( &oi->m_osc[i]->m_oscModel, SIGNAL( dataChanged() ),
			this, SLOT( updateKnobHint() ) );

		oscKnob->setHintText( tr( "Osc %1 waveform:" ).arg( i + 1 ) + " ", QString() );
										
		// setup volume-knob
		knob * volKnob = new knob( knobStyled, this );
		volKnob->setVolumeKnob( true );
		volKnob->move( x + i * colWidth, y + rowHeight*1 );
		volKnob->setFixedSize( 21, 21 );
		volKnob->setHintText( tr( "Osc %1 volume:" ).arg(
							i + 1 ) + " ", "%" );
							
		// setup panning-knob
		knob * panKnob = new organicKnob( this );
		panKnob->move( x + i  * colWidth, y + rowHeight*2 );
		panKnob->setHintText( tr("Osc %1 panning:").arg(
							i + 1 ) + " ", "" );
							
		// setup knob for fine-detuning
		knob * detuneKnob = new organicKnob( this );
		detuneKnob->move( x + i * colWidth, y + rowHeight*3 );
		detuneKnob->setHintText( tr( "Osc %1 stereo detuning" ).arg( i + 1 )
							+ " ", " " +
							tr( "cents" ) );

		m_oscKnobs[i] = OscillatorKnobs( harmKnob, volKnob, oscKnob, panKnob, detuneKnob );

		// Attach to models
		m_oscKnobs[i].m_harmKnob->setModel( &oi->m_osc[i]->m_harmModel );
		m_oscKnobs[i].m_volKnob->setModel( &oi->m_osc[i]->m_volModel );
		m_oscKnobs[i].m_oscKnob->setModel( &oi->m_osc[i]->m_oscModel );
		m_oscKnobs[i].m_panKnob->setModel( &oi->m_osc[i]->m_panModel );
		m_oscKnobs[i].m_detuneKnob->setModel( &oi->m_osc[i]->m_detuneModel );
	}
	updateKnobHint();
}


void organicInstrumentView::updateKnobHint()
{
	organicInstrument * oi = castModel<organicInstrument>();
	for( int i = 0; i < m_numOscillators; ++i )
	{
		const float harm = oi->m_osc[i]->m_harmModel.value();
		const float wave = oi->m_osc[i]->m_oscModel.value();
		
		m_oscKnobs[i].m_harmKnob->setHintText( tr( "Osc %1 harmonic:" ) + " ", " (" +
			HARMONIC_NAMES[ static_cast<int>( harm ) ] + ")" );
		m_oscKnobs[i].m_oscKnob->setHintText( tr( "Osc %1 waveform:" ) + " ", " (" +
			WAVEFORM_NAMES[ static_cast<int>( wave ) ] + ")" );
	}
}




OscillatorObject::OscillatorObject( Model * _parent, int _index ) :
	Model( _parent ),
	m_waveShape( Oscillator::SineWave, 0, Oscillator::NumWaveShapes-1, this ),
	m_oscModel( 0.0f, 0.0f, 5.0f, 1.0f,
			this, tr( "Osc %1 waveform" ).arg( _index + 1 ) ),
	m_harmModel( static_cast<float>( _index ), 0.0f, 17.0f, 1.0f,
			this, tr( "Osc %1 harmonic" ).arg( _index + 1 ) ),
	m_volModel( 100.0f, 0.0f, 100.0f, 1.0f,
			this, tr( "Osc %1 volume" ).arg( _index + 1 ) ),
	m_panModel( DefaultPanning, PanningLeft, PanningRight, 1.0f,
			this, tr( "Osc %1 panning" ).arg( _index + 1 ) ),
	m_detuneModel( 0.0f, -1200.0f, 1200.0f, 1.0f, 
			this, tr( "Osc %1 fine detuning left" ).arg( _index + 1 ) )
{
}




OscillatorObject::~OscillatorObject()
{
}




void OscillatorObject::oscButtonChanged()
{

	static Oscillator::WaveShapes shapes[] =
	{
		Oscillator::SineWave,
		Oscillator::SawWave,
		Oscillator::SquareWave,
		Oscillator::TriangleWave,
		Oscillator::MoogSawWave,
		Oscillator::ExponentialWave
	} ;

	m_waveShape.setValue( shapes[(int)roundf( m_oscModel.value() )] );

}




void OscillatorObject::updateVolume()
{
	m_volumeLeft = ( 1.0f - m_panModel.value() / (float)PanningRight )
			* m_volModel.value() / m_numOscillators / 100.0f;
	m_volumeRight = ( 1.0f + m_panModel.value() / (float)PanningRight )
			* m_volModel.value() / m_numOscillators / 100.0f;
}




void OscillatorObject::updateDetuning()
{
	m_detuningLeft = powf( 2.0f, organicInstrument::s_harmonics[ static_cast<int>( m_harmModel.value() ) ]
				+ (float)m_detuneModel.value() * CENT ) /
				engine::mixer()->processingSampleRate();
	m_detuningRight = powf( 2.0f, organicInstrument::s_harmonics[ static_cast<int>( m_harmModel.value() ) ]
				- (float)m_detuneModel.value() * CENT ) /
				engine::mixer()->processingSampleRate();
}




extern "C"
{

// necessary for getting instance out of shared lib
Plugin * PLUGIN_EXPORT lmms_plugin_main( Model *, void * _data )
{
	return( new organicInstrument( static_cast<InstrumentTrack *>( _data ) ) );
}


}

/*
 * some notes & ideas for the future of this plugin:
 * 
 * - 32.692 Hz in the bass to 5919.85 Hz of treble in  a Hammond organ
 * => implement harmonic foldback
 * 
 m_osc[i].m_oscModel->setInitValue( 0.0f );
 * - randomize preset 
 */



#include "moc_organic.cxx"
