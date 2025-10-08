/*
 * Organic.cpp - additive synthesizer for organ-like sounds
 *
 * Copyright (c) 2006-2008 Andreas Brandmaier <andy/at/brandmaier/dot/de>
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

#include "Organic.h"

#include <QDomElement>

#include "Engine.h"
#include "AudioEngine.h"
#include "InstrumentTrack.h"
#include "Knob.h"
#include "NotePlayHandle.h"
#include "Oscillator.h"
#include "PixmapButton.h"

#include "embed.h"
#include "plugin_export.h"

namespace lmms
{


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT organic_plugin_descriptor =
{
	LMMS_STRINGIFY( PLUGIN_NAME ),
	"Organic",
	QT_TRANSLATE_NOOP( "PluginBrowser",
				"Additive Synthesizer for organ-like sounds" ),
	"Andreas Brandmaier <andreas/at/brandmaier.de>",
	0x0100,
	Plugin::Type::Instrument,
	new PluginPixmapLoader( "logo" ),
	{},
	nullptr,
} ;

}

float * OrganicInstrument::s_harmonics = nullptr;

/***********************************************************************
*
*	class OrganicInstrument
*
*	lmms - plugin
*
***********************************************************************/


OrganicInstrument::OrganicInstrument( InstrumentTrack * _instrument_track ) :
	Instrument( _instrument_track, &organic_plugin_descriptor ),
	m_modulationAlgo(static_cast<int>(Oscillator::ModulationAlgo::SignalMix),
		static_cast<int>(Oscillator::ModulationAlgo::SignalMix),
		static_cast<int>(Oscillator::ModulationAlgo::SignalMix)),
	m_fx1Model( 0.0f, 0.0f, 0.99f, 0.01f , this, tr( "Distortion" ) ),
	m_volModel( 100.0f, 0.0f, 200.0f, 1.0f, this, tr( "Volume" ) )
{
	m_numOscillators = NUM_OSCILLATORS;

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

	if( s_harmonics == nullptr )
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


	connect( Engine::audioEngine(), SIGNAL( sampleRateChanged() ),
					this, SLOT( updateAllDetuning() ) );
}




OrganicInstrument::~OrganicInstrument()
{
	delete[] m_osc;
}




void OrganicInstrument::saveSettings(QDomDocument& doc, QDomElement& elem)
{
	elem.setAttribute("num_osc", QString::number(m_numOscillators));
	m_fx1Model.saveSettings(doc, elem, "foldback");
	m_volModel.saveSettings(doc, elem, "vol");

	for (int i = 0; i < m_numOscillators; ++i)
	{
		const auto is = QString::number(i);
		m_osc[i]->m_volModel.saveSettings(doc, elem, "vol" + is);
		m_osc[i]->m_panModel.saveSettings(doc, elem, "pan" + is);
		m_osc[i]->m_harmModel.saveSettings(doc, elem, "newharmonic" + is);
		m_osc[i]->m_detuneModel.saveSettings(doc, elem, "newdetune" + is);
		m_osc[i]->m_oscModel.saveSettings(doc, elem, "wavetype" + is);
	}
}




void OrganicInstrument::loadSettings(const QDomElement& elem)
{
	for (int i = 0; i < m_numOscillators; ++i)
	{
		const auto is = QString::number(i);

		m_osc[i]->m_volModel.loadSettings(elem, "vol" + is);

		if (elem.hasAttribute("detune" + is) || !elem.firstChildElement("detune" + is).isNull())
		{
			m_osc[i]->m_detuneModel.loadSettings(elem, "detune" + is);
			m_osc[i]->m_detuneModel.setValue(m_osc[i]->m_detuneModel.value() * 12); // compat
		}
		else
		{
			m_osc[i]->m_detuneModel.loadSettings(elem, "newdetune" + is);
		}

		m_osc[i]->m_panModel.loadSettings(elem, "pan" + is);
		m_osc[i]->m_oscModel.loadSettings(elem, "wavetype" + is);

		if (elem.hasAttribute("newharmonic" + is) || !elem.firstChildElement("newharmonic" + is).isNull())
		{
			m_osc[i]->m_harmModel.loadSettings(elem, "newharmonic" + is);
		}
		else
		{
			m_osc[i]->m_harmModel.setValue(static_cast<float>(i));
		}
	}

	m_volModel.loadSettings(elem, "vol");
	m_fx1Model.loadSettings(elem, "foldback");
}


QString OrganicInstrument::nodeName() const
{
	return( organic_plugin_descriptor.name );
}




void OrganicInstrument::playNote( NotePlayHandle * _n,
						SampleFrame* _working_buffer )
{
	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	const f_cnt_t offset = _n->noteOffset();

	if (!_n->m_pluginData)
	{
		auto oscs_l = std::array<Oscillator*, NUM_OSCILLATORS>{};
		auto oscs_r = std::array<Oscillator*, NUM_OSCILLATORS>{};

		_n->m_pluginData = new oscPtr;

		for( int i = m_numOscillators - 1; i >= 0; --i )
		{
			static_cast<oscPtr *>( _n->m_pluginData )->phaseOffsetLeft[i]
				= rand() / (static_cast<float>(RAND_MAX) + 1.0f);
			static_cast<oscPtr *>( _n->m_pluginData )->phaseOffsetRight[i]
				= rand() / (static_cast<float>(RAND_MAX) + 1.0f);

			// initialise ocillators

			if( i == m_numOscillators - 1 )
			{
				// create left oscillator
				oscs_l[i] = new Oscillator(
						&m_osc[i]->m_waveShape,
						&m_modulationAlgo,
						_n->frequency(),
						m_osc[i]->m_detuningLeft,
						static_cast<oscPtr *>( _n->m_pluginData )->phaseOffsetLeft[i],
						m_osc[i]->m_volumeLeft );
				// create right oscillator
				oscs_r[i] = new Oscillator(
						&m_osc[i]->m_waveShape,
						&m_modulationAlgo,
						_n->frequency(),
						m_osc[i]->m_detuningRight,
						static_cast<oscPtr *>( _n->m_pluginData )->phaseOffsetRight[i],
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
						static_cast<oscPtr *>( _n->m_pluginData )->phaseOffsetLeft[i],
						m_osc[i]->m_volumeLeft,
						oscs_l[i + 1] );
				// create right oscillator
				oscs_r[i] = new Oscillator(
						&m_osc[i]->m_waveShape,
						&m_modulationAlgo,
						_n->frequency(),
						m_osc[i]->m_detuningRight,
						static_cast<oscPtr *>( _n->m_pluginData )->phaseOffsetRight[i],
						m_osc[i]->m_volumeRight,
						oscs_r[i + 1] );
			}


		}

		static_cast<oscPtr *>( _n->m_pluginData )->oscLeft = oscs_l[0];
		static_cast<oscPtr *>( _n->m_pluginData )->oscRight = oscs_r[0];
	}

	Oscillator * osc_l = static_cast<oscPtr *>( _n->m_pluginData )->oscLeft;
	Oscillator * osc_r = static_cast<oscPtr *>( _n->m_pluginData)->oscRight;

	osc_l->update( _working_buffer + offset, frames, 0 );
	osc_r->update( _working_buffer + offset, frames, 1 );


	// -- fx section --

	// fxKnob is [0;1]
	float t =  m_fx1Model.value();

	for (auto i = std::size_t{0}; i < frames + offset; i++)
	{
		_working_buffer[i][0] = waveshape( _working_buffer[i][0], t ) *
						m_volModel.value() / 100.0f;
		_working_buffer[i][1] = waveshape( _working_buffer[i][1], t ) *
						m_volModel.value() / 100.0f;
	}

	// -- --
}




void OrganicInstrument::deleteNotePluginData( NotePlayHandle * _n )
{
	delete static_cast<Oscillator *>( static_cast<oscPtr *>(
						_n->m_pluginData )->oscLeft );
	delete static_cast<Oscillator *>( static_cast<oscPtr *>(
						_n->m_pluginData )->oscRight );

	delete static_cast<oscPtr *>( _n->m_pluginData );
}

/*float inline OrganicInstrument::foldback(float in, float threshold)
{
  if (in>threshold || in<-threshold)
  {
    in= fabs(fabs(fmod(in - threshold, threshold*4)) - threshold*2) - threshold;
  }
  return in;
}
*/




float inline OrganicInstrument::waveshape(float in, float amount)
{
	float k = 2.0f * amount / ( 1.0f - amount );
	return (1.0f + k) * in / (1.0f + k * std::abs(in));
}




void OrganicInstrument::randomiseSettings()
{

	for( int i = 0; i < m_numOscillators; i++ )
	{
		m_osc[i]->m_volModel.setValue( intRand( 0, 100 ) );

		m_osc[i]->m_detuneModel.setValue( intRand( -5, 5 ) );

		m_osc[i]->m_panModel.setValue( 0 );

		m_osc[i]->m_oscModel.setValue( intRand( 0, 5 ) );
	}

}




void OrganicInstrument::updateAllDetuning()
{
	for( int i = 0; i < m_numOscillators; ++i )
	{
		m_osc[i]->updateDetuning();
	}
}




int OrganicInstrument::intRand( int min, int max )
{
//	int randn = min+int((max-min)*rand()/(RAND_MAX + 1.0));
//	cout << randn << endl;
	int randn = ( rand() % (max - min) ) + min;
	return( randn );
}


gui::PluginView * OrganicInstrument::instantiateView( QWidget * _parent )
{
	return( new gui::OrganicInstrumentView( this, _parent ) );
}


namespace gui
{


class OrganicKnob : public Knob
{
public:
	OrganicKnob( QWidget * _parent ) :
		Knob( KnobType::Styled, _parent )
	{
		setFixedSize( 21, 21 );
	}
};



OrganicInstrumentView::OrganicInstrumentView( Instrument * _instrument,
							QWidget * _parent ) :
	InstrumentViewFixedSize( _instrument, _parent ),
	m_oscKnobs( nullptr )
{
	auto oi = castModel<OrganicInstrument>();

	setAutoFillBackground( true );
	QPalette pal;
	static auto s_artwork = PLUGIN_NAME::getIconPixmap("artwork");
	pal.setBrush(backgroundRole(), s_artwork);
	setPalette( pal );

	// setup knob for FX1
	m_fx1Knob = new OrganicKnob( this );
	m_fx1Knob->move( 15, 201 );
	m_fx1Knob->setFixedSize( 37, 47 );
	m_fx1Knob->setHintText( tr( "Distortion:" ), QString() );
	m_fx1Knob->setObjectName( "fx1Knob" );

	// setup volume-knob
	m_volKnob = new OrganicKnob( this );
	m_volKnob->setVolumeKnob( true );
	m_volKnob->move( 60, 201 );
	m_volKnob->setFixedSize( 37, 47 );
	m_volKnob->setHintText( tr( "Volume:" ), "%" );
	m_volKnob->setObjectName( "volKnob" );

	// randomise
	m_randBtn = new PixmapButton( this, tr( "Randomise" ) );
	m_randBtn->move( 148, 224 );
	m_randBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"randomise_pressed" ) );
	m_randBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
								"randomise" ) );

	connect( m_randBtn, SIGNAL ( clicked() ),
					oi, SLOT( randomiseSettings() ) );


}


OrganicInstrumentView::~OrganicInstrumentView()
{
	delete[] m_oscKnobs;
}


void OrganicInstrumentView::modelChanged()
{
	auto oi = castModel<OrganicInstrument>();

	const float y=91.0f;
	const float rowHeight = 26.0f;
	const float x=53.0f;
	const float colWidth = 24.0f;

	m_numOscillators = oi->m_numOscillators;

	m_fx1Knob->setModel( &oi->m_fx1Model );
	m_volKnob->setModel( &oi->m_volModel );

	if( m_oscKnobs != nullptr )
	{
		delete[] m_oscKnobs;
	}

	m_oscKnobs = new OscillatorKnobs[ m_numOscillators ];

	// Create knobs, now that we know how many to make
	for( int i = 0; i < m_numOscillators; ++i )
	{
		// setup harmonic knob
		Knob * harmKnob = new OrganicKnob( this );
		harmKnob->move( x + i * colWidth, y - rowHeight );
		harmKnob->setObjectName( "harmKnob" );
		connect( &oi->m_osc[i]->m_harmModel, SIGNAL( dataChanged() ),
			this, SLOT( updateKnobHint() ) );

		// setup waveform-knob
		Knob * oscKnob = new OrganicKnob( this );
		oscKnob->move( x + i * colWidth, y );
		connect( &oi->m_osc[i]->m_oscModel, SIGNAL( dataChanged() ),
			this, SLOT( updateKnobHint() ) );

		oscKnob->setHintText( tr( "Osc %1 waveform:" ).arg( i + 1 ), QString() );

		// setup volume-knob
		auto volKnob = new Knob(KnobType::Styled, this);
		volKnob->setVolumeKnob( true );
		volKnob->move( x + i * colWidth, y + rowHeight*1 );
		volKnob->setFixedSize( 21, 21 );
		volKnob->setHintText( tr( "Osc %1 volume:" ).arg(
								i + 1 ), "%" );

		// setup panning-knob
		Knob * panKnob = new OrganicKnob( this );
		panKnob->move( x + i  * colWidth, y + rowHeight*2 );
		panKnob->setHintText( tr("Osc %1 panning:").arg(
								i + 1 ), "" );

		// setup knob for fine-detuning
		Knob * detuneKnob = new OrganicKnob( this );
		detuneKnob->move( x + i * colWidth, y + rowHeight*3 );
		detuneKnob->setHintText( tr( "Osc %1 stereo detuning" ).arg( i + 1 )
							, " " +
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


void OrganicInstrumentView::updateKnobHint()
{
	auto oi = castModel<OrganicInstrument>();
	for( int i = 0; i < m_numOscillators; ++i )
	{
		const float harm = oi->m_osc[i]->m_harmModel.value();
		const float wave = oi->m_osc[i]->m_oscModel.value();

		m_oscKnobs[i].m_harmKnob->setHintText( tr( "Osc %1 harmonic:" ).arg( i + 1 ), " (" +
			HARMONIC_NAMES[ static_cast<int>( harm ) ] + ")" );
		m_oscKnobs[i].m_oscKnob->setHintText( tr( "Osc %1 waveform:" ).arg( i + 1 ), " (" +
			WAVEFORM_NAMES[ static_cast<int>( wave ) ] + ")" );
	}
}


} // namespace gui


OscillatorObject::OscillatorObject( Model * _parent, int _index ) :
	Model( _parent ),
	m_waveShape( static_cast<int>(Oscillator::WaveShape::Sine), 0, Oscillator::NumWaveShapes-1, this ),
	m_oscModel( 0.0f, 0.0f, 5.0f, 1.0f,
			this, tr( "Osc %1 waveform" ).arg( _index + 1 ) ),
	m_harmModel( static_cast<float>( _index ), 0.0f, 17.0f, 1.0f,
			this, tr( "Osc %1 harmonic" ).arg( _index + 1 ) ),
	m_volModel( 100.0f, 0.0f, 100.0f, 1.0f,
			this, tr( "Osc %1 volume" ).arg( _index + 1 ) ),
	m_panModel( DefaultPanning, PanningLeft, PanningRight, 1.0f,
			this, tr( "Osc %1 panning" ).arg( _index + 1 ) ),
	m_detuneModel( 0.0f, -1200.0f, 1200.0f, 1.0f,
			this, tr( "Osc %1 stereo detuning" ).arg( _index + 1 ) )
{
}




void OscillatorObject::oscButtonChanged()
{

	static auto shapes = std::array
	{
		Oscillator::WaveShape::Sine,
		Oscillator::WaveShape::Saw,
		Oscillator::WaveShape::Square,
		Oscillator::WaveShape::Triangle,
		Oscillator::WaveShape::MoogSaw,
		Oscillator::WaveShape::Exponential
	} ;

	m_waveShape.setValue( static_cast<float>(shapes[(int)roundf( m_oscModel.value() )]) );

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
	const auto harmonic = OrganicInstrument::s_harmonics[static_cast<int>(m_harmModel.value())];
	const auto sr = Engine::audioEngine()->outputSampleRate();

	m_detuningLeft = std::exp2(harmonic + m_detuneModel.value() * CENT) / sr;
	m_detuningRight = std::exp2(harmonic - m_detuneModel.value() * CENT) / sr;
}


extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model *m, void * )
{
	return( new OrganicInstrument( static_cast<InstrumentTrack *>( m ) ) );
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


} // namespace lmms
