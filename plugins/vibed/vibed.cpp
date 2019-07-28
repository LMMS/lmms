/*
 * vibed.cpp - combination of PluckedStringSynth and BitInvader
 *
 * Copyright (c) 2006-2008 Danny McRae <khjklujn/at/yahoo/com>
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

#include <QDomDocument>
#include <QMap>

#include "vibed.h"
#include "Engine.h"
#include "InstrumentTrack.h"
#include "Mixer.h"
#include "NotePlayHandle.h"
#include "ToolTip.h"
#include "base64.h"
#include "CaptionMenu.h"
#include "Oscillator.h"
#include "string_container.h"
#include "volume.h"
#include "Song.h"

#include "embed.h"
#include "plugin_export.h"

extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT vibedstrings_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"Vibed",
	QT_TRANSLATE_NOOP( "pluginBrowser",
					"Vibrating string modeler" ),
	"Danny McRae <khjklujn/at/yahoo/com>",
	0x0100,
	Plugin::Instrument,
	new PluginPixmapLoader( "logo" ),
	NULL,
	NULL
};

}


vibed::vibed( InstrumentTrack * _instrumentTrack ) :
	Instrument( _instrumentTrack, &vibedstrings_plugin_descriptor )
{

	FloatModel * knob;
	BoolModel * led;
	nineButtonSelectorModel * harmonic;
	graphModel * graphTmp;

	for( int harm = 0; harm < 9; harm++ )
	{
		knob = new FloatModel( DefaultVolume, MinVolume, MaxVolume,
				1.0f, this, tr( "String %1 volume" ).arg( harm+1 ) );
		m_volumeKnobs.append( knob );

		knob = new FloatModel( 0.0f, 0.0f, 0.05f, 0.001f, this,
				tr( "String %1 stiffness" ).arg( harm+1 ) );
		m_stiffnessKnobs.append( knob );

		knob = new FloatModel( 0.0f, 0.0f, 0.05f, 0.005f, this,
				tr( "Pick %1 position" ).arg( harm+1 )  );
		m_pickKnobs.append( knob );

		knob = new FloatModel( 0.05f, 0.0f, 0.05f, 0.005f, this,
				tr( "Pickup %1 position" ).arg( harm+1 ) );
		m_pickupKnobs.append( knob );

		knob = new FloatModel( 0.0f, -1.0f, 1.0f, 0.01f, this,
				tr( "String %1 panning" ).arg( harm+1 )  );
		m_panKnobs.append( knob );

		knob = new FloatModel( 0.0f, -0.1f, 0.1f, 0.001f, this,
				tr( "String %1 detune" ).arg( harm+1 ) );
		m_detuneKnobs.append( knob );

		knob = new FloatModel( 0.0f, 0.0f, 0.75f, 0.01f, this,
				tr( "String %1 fuzziness" ).arg( harm+1 ) );
		m_randomKnobs.append( knob );

		knob = new FloatModel( 1, 1, 16, 1, this,
				tr( "String %1 length" ).arg( harm+1 ) );
		m_lengthKnobs.append( knob );

		led = new BoolModel( false, this,
				tr( "Impulse %1" ).arg( harm+1 ) );
		m_impulses.append( led );

		led = new BoolModel( harm==0, this,
				tr( "String %1" ).arg( harm+1 )  );
		m_powerButtons.append( led );

		harmonic = new nineButtonSelectorModel( 2, 0, 8, this );
		m_harmonics.append( harmonic );

		graphTmp = new graphModel( -1.0, 1.0, __sampleLength, this );
		graphTmp->setWaveToSine();

		m_graphs.append( graphTmp );

	}
}




vibed::~vibed()
{
}




void vibed::saveSettings( QDomDocument & _doc, QDomElement & _this )
{

	QString name;
	
	// Save plugin version
	_this.setAttribute( "version", "0.1" );
	
	for( int i = 0; i < 9; i++ )
	{
		name = "active" + QString::number( i );
		_this.setAttribute( name, QString::number(
				m_powerButtons[i]->value() ) );

		if( m_powerButtons[i]->value() )
		{
			name = "volume" + QString::number( i );
			m_volumeKnobs[i]->saveSettings( _doc, _this, name );
	
			name = "stiffness" + QString::number( i );
			m_stiffnessKnobs[i]->saveSettings( _doc, _this, name );

			name = "pick" + QString::number( i );
			m_pickKnobs[i]->saveSettings( _doc, _this, name );

			name = "pickup" + QString::number( i );
			m_pickupKnobs[i]->saveSettings( _doc, _this, name );

			name = "octave" + QString::number( i );
			m_harmonics[i]->saveSettings( _doc, _this, name );

			name = "length" + QString::number( i );
			m_lengthKnobs[i]->saveSettings( _doc, _this, name );

			name = "pan" + QString::number( i );
			m_panKnobs[i]->saveSettings( _doc, _this, name );

			name = "detune" + QString::number( i );
			m_detuneKnobs[i]->saveSettings( _doc, _this, name );

			name = "slap" + QString::number( i );
			m_randomKnobs[i]->saveSettings( _doc, _this, name );

			name = "impulse" + QString::number( i );
			m_impulses[i]->saveSettings( _doc, _this, name );

			QString sampleString;
			base64::encode(
				(const char *)m_graphs[i]->samples(),
				__sampleLength * sizeof(float),
				sampleString );
			name = "graph" + QString::number( i );
			_this.setAttribute( name, sampleString );
		}
	}

}



void vibed::loadSettings( const QDomElement & _this )
{

	QString name;

	for( int i = 0; i < 9; i++ )
	{
		name = "active" + QString::number( i );
		m_powerButtons[i]->setValue( _this.attribute( name ).toInt() );
		
		if( m_powerButtons[i]->value() &&
			_this.hasAttribute( "volume" + QString::number( i ) ) )
		{
			name = "volume" + QString::number( i );
			m_volumeKnobs[i]->loadSettings( _this, name );
		
			name = "stiffness" + QString::number( i );
			m_stiffnessKnobs[i]->loadSettings( _this, name );
		
			name = "pick" + QString::number( i );
			m_pickKnobs[i]->loadSettings( _this, name );
		
			name = "pickup" + QString::number( i );
			m_pickupKnobs[i]->loadSettings( _this, name );
		
			name = "octave" + QString::number( i );
			m_harmonics[i]->loadSettings( _this, name );
			
			name = "length" + QString::number( i );
			m_lengthKnobs[i]->loadSettings( _this, name );
		
			name = "pan" + QString::number( i );
			m_panKnobs[i]->loadSettings( _this, name );
		
			name = "detune" + QString::number( i );
			m_detuneKnobs[i]->loadSettings( _this, name );
		
			name = "slap" + QString::number( i );
			m_randomKnobs[i]->loadSettings( _this, name );
		
			name = "impulse" + QString::number( i );
			m_impulses[i]->loadSettings( _this, name );

			int size = 0;
			float * shp = 0;
			base64::decode( _this.attribute( "graph" +
						QString::number( i ) ),
						&shp,
						&size );
			// TODO: check whether size == 128 * sizeof( float ),
			// otherwise me might and up in a segfault
			m_graphs[i]->setSamples( shp );
			delete[] shp;
			

			// TODO: do one of the following to avoid
			// "uninitialized" wave-shape-buttongroup
			// - activate random-wave-shape-button here
			// - make wave-shape-buttons simple toggle-buttons
			//   instead of checkable buttons
			// - save and restore selected wave-shape-button
		}
	}
	
//	update();
}




QString vibed::nodeName() const
{
	return( vibedstrings_plugin_descriptor.name );
}




void vibed::playNote( NotePlayHandle * _n, sampleFrame * _working_buffer )
{
	if ( _n->totalFramesPlayed() == 0 || _n->m_pluginData == NULL )
	{
		_n->m_pluginData = new stringContainer( _n->frequency(),
				Engine::mixer()->processingSampleRate(),
						__sampleLength );
		
		for( int i = 0; i < 9; ++i )
		{
			if( m_powerButtons[i]->value() )
			{
				static_cast<stringContainer*>(
					_n->m_pluginData )->addString(
				m_harmonics[i]->value(),
				m_pickKnobs[i]->value(),
				m_pickupKnobs[i]->value(),
				m_graphs[i]->samples(),
				m_randomKnobs[i]->value(),
				m_stiffnessKnobs[i]->value(),
				m_detuneKnobs[i]->value(),
				static_cast<int>(
					m_lengthKnobs[i]->value() ),
				m_impulses[i]->value(),
				i );
			}
		}
	}

	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	const f_cnt_t offset = _n->noteOffset();
	stringContainer * ps = static_cast<stringContainer *>(
							_n->m_pluginData );

	for( fpp_t i = offset; i < frames + offset; ++i )
	{
		_working_buffer[i][0] = 0.0f;
		_working_buffer[i][1] = 0.0f;
		int s = 0;
		for( int string = 0; string < 9; ++string )
		{
			if( ps->exists( string ) )
			{
				// pan: 0 -> left, 1 -> right
				const float pan = ( m_panKnobs[string]->value() + 1 ) / 2.0f;
				const sample_t sample = ps->getStringSample( s ) * m_volumeKnobs[string]->value() / 100.0f;
				_working_buffer[i][0] += ( 1.0f - pan ) * sample;
				_working_buffer[i][1] += pan * sample;
				s++;
			}
		}
	}

	instrumentTrack()->processAudioBuffer( _working_buffer, frames + offset, _n );
}




void vibed::deleteNotePluginData( NotePlayHandle * _n )
{
	delete static_cast<stringContainer *>( _n->m_pluginData );
}




PluginView * vibed::instantiateView( QWidget * _parent )
{
	return( new vibedView( this, _parent ) );
}





vibedView::vibedView( Instrument * _instrument,
				QWidget * _parent ) :
	InstrumentViewFixedSize( _instrument, _parent )
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap(
								"artwork" ) );
	setPalette( pal );
	
	m_volumeKnob = new Knob( knobBright_26, this );
	m_volumeKnob->setVolumeKnob( true );
	m_volumeKnob->move( 103, 142 );
	m_volumeKnob->setHintText( tr( "String volume:" ), "" );

	m_stiffnessKnob = new Knob( knobBright_26, this );
	m_stiffnessKnob->move( 129, 142 );
	m_stiffnessKnob->setHintText( tr( "String stiffness:" )
							, "" );
	
	
	m_pickKnob = new Knob( knobBright_26, this );
	m_pickKnob->move( 153, 142 );
	m_pickKnob->setHintText( tr( "Pick position:" ), "" );

	m_pickupKnob = new Knob( knobBright_26, this );
	m_pickupKnob->move( 177, 142 );
	m_pickupKnob->setHintText( tr( "Pickup position:" )
						, "" );

	m_panKnob = new Knob( knobBright_26, this );
	m_panKnob->move( 105, 187 );
    m_panKnob->setHintText( tr( "String panning:" ), "" );
	
	m_detuneKnob = new Knob( knobBright_26, this );
	m_detuneKnob->move( 150, 187 );
	m_detuneKnob->setHintText( tr( "String detune:" ), "" );

	m_randomKnob = new Knob( knobBright_26, this );
	m_randomKnob->move( 194, 187 );
	m_randomKnob->setHintText( tr( "String fuzziness:" )
						, "" );

	m_lengthKnob = new Knob( knobBright_26, this );
	m_lengthKnob->move( 23, 193 );
	m_lengthKnob->setHintText( tr( "String length:" )
						, "" );

	m_impulse = new LedCheckBox( "", this );
	m_impulse->move( 23, 94 );
	ToolTip::add( m_impulse,
			tr( "Impulse" ) );

	m_harmonic = new nineButtonSelector(
		PLUGIN_NAME::getIconPixmap( "button_-2_on" ),
		PLUGIN_NAME::getIconPixmap( "button_-2_off" ),
		PLUGIN_NAME::getIconPixmap( "button_-1_on" ),
		PLUGIN_NAME::getIconPixmap( "button_-1_off" ),
		PLUGIN_NAME::getIconPixmap( "button_f_on" ),
		PLUGIN_NAME::getIconPixmap( "button_f_off" ),
		PLUGIN_NAME::getIconPixmap( "button_2_on" ),
		PLUGIN_NAME::getIconPixmap( "button_2_off" ),
		PLUGIN_NAME::getIconPixmap( "button_3_on" ),
		PLUGIN_NAME::getIconPixmap( "button_3_off" ),
		PLUGIN_NAME::getIconPixmap( "button_4_on" ),
		PLUGIN_NAME::getIconPixmap( "button_4_off" ),
		PLUGIN_NAME::getIconPixmap( "button_5_on" ),
		PLUGIN_NAME::getIconPixmap( "button_5_off" ),
		PLUGIN_NAME::getIconPixmap( "button_6_on" ),
		PLUGIN_NAME::getIconPixmap( "button_6_off" ),
		PLUGIN_NAME::getIconPixmap( "button_7_on" ),
		PLUGIN_NAME::getIconPixmap( "button_7_off" ),
		2,
		21, 127,
		this );

	m_harmonic->setWindowTitle( tr( "Octave" ) );
	

	m_stringSelector = new nineButtonSelector(
			PLUGIN_NAME::getIconPixmap( "button_1_on" ),
			PLUGIN_NAME::getIconPixmap( "button_1_off" ),
			PLUGIN_NAME::getIconPixmap( "button_2_on" ),
			PLUGIN_NAME::getIconPixmap( "button_2_off" ),
			PLUGIN_NAME::getIconPixmap( "button_3_on" ),
			PLUGIN_NAME::getIconPixmap( "button_3_off" ),
			PLUGIN_NAME::getIconPixmap( "button_4_on" ),
			PLUGIN_NAME::getIconPixmap( "button_4_off" ),
			PLUGIN_NAME::getIconPixmap( "button_5_on" ),
			PLUGIN_NAME::getIconPixmap( "button_5_off" ),
			PLUGIN_NAME::getIconPixmap( "button_6_on" ),
			PLUGIN_NAME::getIconPixmap( "button_6_off" ),
			PLUGIN_NAME::getIconPixmap( "button_7_on" ),
			PLUGIN_NAME::getIconPixmap( "button_7_off" ),
			PLUGIN_NAME::getIconPixmap( "button_8_on" ),
			PLUGIN_NAME::getIconPixmap( "button_8_off" ),
			PLUGIN_NAME::getIconPixmap( "button_9_on" ),
			PLUGIN_NAME::getIconPixmap( "button_9_off" ),
			0,
			21, 39,
			this);

	m_graph = new Graph( this );
	m_graph->setWindowTitle( tr( "Impulse Editor" ) );
	m_graph->setForeground( PLUGIN_NAME::getIconPixmap( "wavegraph4" ) );
	m_graph->move( 76, 21 );
	m_graph->resize(132, 104);


	m_power = new LedCheckBox( "", this, tr( "Enable waveform" ) );
	m_power->move( 212, 130 );
	ToolTip::add( m_power,
			tr( "Enable/disable string" ) );

	
	// String selector is not a part of the model
	m_stringSelector->setWindowTitle( tr( "String" ) );

	connect( m_stringSelector, SIGNAL( nineButtonSelection( int ) ),
			this, SLOT( showString( int ) ) );

	showString( 0 );

	m_sinWaveBtn = new PixmapButton( this, tr( "Sine wave" ) );
	m_sinWaveBtn->move( 212, 24 );
	m_sinWaveBtn->setActiveGraphic( embed::getIconPixmap(
				"sin_wave_active" ) );
	m_sinWaveBtn->setInactiveGraphic( embed::getIconPixmap(
				"sin_wave_inactive" ) );
	ToolTip::add( m_sinWaveBtn,
				tr( "Sine wave" ) );
	connect( m_sinWaveBtn, SIGNAL (clicked () ),
			this, SLOT ( sinWaveClicked() ) );

	
	m_triangleWaveBtn = new PixmapButton( this, tr( "Triangle wave" ) );
	m_triangleWaveBtn->move( 212, 41 );
	m_triangleWaveBtn->setActiveGraphic(
			embed::getIconPixmap( "triangle_wave_active" ) );
	m_triangleWaveBtn->setInactiveGraphic(
			embed::getIconPixmap( "triangle_wave_inactive" ) );
	ToolTip::add( m_triangleWaveBtn,
			tr( "Triangle wave" ) );
	connect( m_triangleWaveBtn, SIGNAL ( clicked () ),
			this, SLOT ( triangleWaveClicked( ) ) );

	
	m_sawWaveBtn = new PixmapButton( this, tr( "Saw wave" ) );
	m_sawWaveBtn->move( 212, 58 );
	m_sawWaveBtn->setActiveGraphic( embed::getIconPixmap(
				"saw_wave_active" ) );
	m_sawWaveBtn->setInactiveGraphic( embed::getIconPixmap(
				"saw_wave_inactive" ) );
	ToolTip::add( m_sawWaveBtn,
				tr( "Saw wave" ) );
	connect( m_sawWaveBtn, SIGNAL (clicked () ),
			this, SLOT ( sawWaveClicked() ) );

	
	m_sqrWaveBtn = new PixmapButton( this, tr( "Square wave" ) );
	m_sqrWaveBtn->move( 212, 75 );
	m_sqrWaveBtn->setActiveGraphic( embed::getIconPixmap(
				"square_wave_active" ) );
	m_sqrWaveBtn->setInactiveGraphic( embed::getIconPixmap(
				"square_wave_inactive" ) );
	ToolTip::add( m_sqrWaveBtn,
			tr( "Square wave" ) );
	connect( m_sqrWaveBtn, SIGNAL ( clicked () ),
			this, SLOT ( sqrWaveClicked() ) );

	
	m_whiteNoiseWaveBtn = new PixmapButton( this, tr( "White noise" ) );
	m_whiteNoiseWaveBtn->move( 212, 92 );
	m_whiteNoiseWaveBtn->setActiveGraphic(
			embed::getIconPixmap( "white_noise_wave_active" ) );
	m_whiteNoiseWaveBtn->setInactiveGraphic(
			embed::getIconPixmap( "white_noise_wave_inactive" ) );
	ToolTip::add( m_whiteNoiseWaveBtn,
			tr( "White noise" ) );
	connect( m_whiteNoiseWaveBtn, SIGNAL ( clicked () ),
			this, SLOT ( noiseWaveClicked() ) );

	
	m_usrWaveBtn = new PixmapButton( this, tr( "User-defined wave" ) );
	m_usrWaveBtn->move( 212, 109 );
	m_usrWaveBtn->setActiveGraphic( embed::getIconPixmap(
				"usr_wave_active" ) );
	m_usrWaveBtn->setInactiveGraphic( embed::getIconPixmap(
				"usr_wave_inactive" ) );
	ToolTip::add( m_usrWaveBtn,
			tr( "User-defined wave" ) );
	connect( m_usrWaveBtn, SIGNAL ( clicked () ),
			this, SLOT ( usrWaveClicked() ) );


	m_smoothBtn = new PixmapButton( this, tr( "Smooth waveform" ) );
	m_smoothBtn->move( 79, 129 );
	m_smoothBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
			"smooth_active" ) );
	m_smoothBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
			"smooth_inactive" ) );
	m_smoothBtn->setChecked( false );
	ToolTip::add( m_smoothBtn,
			tr( "Smooth waveform" ) );
	connect( m_smoothBtn, SIGNAL ( clicked () ),
			this, SLOT ( smoothClicked() ) );
	
	m_normalizeBtn = new PixmapButton( this, tr( "Normalize waveform" ) );
	m_normalizeBtn->move( 96, 129 );
	m_normalizeBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
			"normalize_active" ) );
	m_normalizeBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
			"normalize_inactive" ) );
	m_normalizeBtn->setChecked( false );
	ToolTip::add( m_normalizeBtn,
			tr( "Normalize waveform" ) );

	connect( m_normalizeBtn, SIGNAL ( clicked () ),
			this, SLOT ( normalizeClicked() ) );

}




void vibedView::modelChanged()
{
	showString( 0 );
}




void vibedView::showString( int _string )
{
	vibed * v = castModel<vibed>();
	
	m_pickKnob->setModel( v->m_pickKnobs[_string] );
	m_pickupKnob->setModel( v->m_pickupKnobs[_string] );
	m_stiffnessKnob->setModel( v->m_stiffnessKnobs[_string] );
	m_volumeKnob->setModel( v->m_volumeKnobs[_string] );
	m_panKnob->setModel( v->m_panKnobs[_string] );
	m_detuneKnob->setModel( v->m_detuneKnobs[_string] );
	m_randomKnob->setModel( v->m_randomKnobs[_string] );
	m_lengthKnob->setModel( v->m_lengthKnobs[_string] );
	m_graph->setModel( v->m_graphs[_string] );
	m_impulse->setModel( v->m_impulses[_string] );
	m_harmonic->setModel( v->m_harmonics[_string] );
	m_power->setModel( v->m_powerButtons[_string] );

}




void vibedView::sinWaveClicked()
{
	m_graph->model()->setWaveToSine();
	Engine::getSong()->setModified();
}



void vibedView::triangleWaveClicked()
{
	m_graph->model()->setWaveToTriangle();
	Engine::getSong()->setModified();
}



void vibedView::sawWaveClicked()
{
	m_graph->model()->setWaveToSaw();
	Engine::getSong()->setModified();
}



void vibedView::sqrWaveClicked()
{
	m_graph->model()->setWaveToSquare();
	Engine::getSong()->setModified();
}



void vibedView::noiseWaveClicked()
{
	m_graph->model()->setWaveToNoise();
	Engine::getSong()->setModified();
}



void vibedView::usrWaveClicked()
{
	QString fileName = m_graph->model()->setWaveToUser();
	ToolTip::add( m_usrWaveBtn, fileName );
	Engine::getSong()->setModified();
}



void vibedView::smoothClicked()
{
	m_graph->model()->smooth();
	Engine::getSong()->setModified();
}



void vibedView::normalizeClicked()
{
	m_graph->model()->normalize();
	Engine::getSong()->setModified();
}




void vibedView::contextMenuEvent( QContextMenuEvent * )
{

	CaptionMenu contextMenu( model()->displayName(), this );
	contextMenu.exec( QCursor::pos() );

}


extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model *m, void * )
{
	return( new vibed( static_cast<InstrumentTrack *>( m ) ) );
}


}



