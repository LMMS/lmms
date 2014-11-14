/*
 * vibed.cpp - combination of PluckedStringSynth and BitInvader
 *
 * Copyright (c) 2006-2008 Danny McRae <khjklujn/at/yahoo/com>
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

#include <QtXml/QDomDocument>
#include <QtCore/QMap>
#include <QtGui/QWhatsThis>

#include "vibed.h"
#include "engine.h"
#include "InstrumentTrack.h"
#include "NotePlayHandle.h"
#include "tooltip.h"
#include "base64.h"
#include "caption_menu.h"
#include "Oscillator.h"
#include "string_container.h"
#include "templates.h"
#include "volume.h"
#include "song.h"

#include "embed.cpp"


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
				tr( "Pan %1" ).arg( harm+1 )  );
		m_panKnobs.append( knob );

		knob = new FloatModel( 0.0f, -0.1f, 0.1f, 0.001f, this,
				tr( "Detune %1" ).arg( harm+1 ) );
		m_detuneKnobs.append( knob );

		knob = new FloatModel( 0.0f, 0.0f, 0.75f, 0.01f, this,
				tr( "Fuzziness %1 " ).arg( harm+1 ) );
		m_randomKnobs.append( knob );

		knob = new FloatModel( 1, 1, 16, 1, this,
				tr( "Length %1" ).arg( harm+1 ) );
		m_lengthKnobs.append( knob );

		led = new BoolModel( false, this,
				tr( "Impulse %1" ).arg( harm+1 ) );
		m_impulses.append( led );

		led = new BoolModel( harm==0, this,
				tr( "Octave %1" ).arg( harm+1 )  );
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
				engine::mixer()->processingSampleRate(),
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
	stringContainer * ps = static_cast<stringContainer *>(
							_n->m_pluginData );

	for( fpp_t i = 0; i < frames; ++i )
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

	instrumentTrack()->processAudioBuffer( _working_buffer, frames, _n );
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
	InstrumentView( _instrument, _parent )
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap(
								"artwork" ) );
	setPalette( pal );
	
	m_volumeKnob = new knob( knobBright_26, this );
	m_volumeKnob->setVolumeKnob( true );
	m_volumeKnob->move( 103, 142 );
	m_volumeKnob->setHintText( tr( "Volume:" ) + " ", "" );
	m_volumeKnob->setWhatsThis( tr( "The 'V' knob sets the volume "
					"of the selected string." ) );

	m_stiffnessKnob = new knob( knobBright_26, this );
	m_stiffnessKnob->move( 129, 142 );
	m_stiffnessKnob->setHintText( tr( "String stiffness:" ) +
					" ", "" );
	m_stiffnessKnob->setWhatsThis( tr(
"The 'S' knob sets the stiffness of the selected string.  The stiffness "
"of the string affects how long the string will ring out.  The lower "
"the setting, the longer the string will ring." ) );
	
	
	m_pickKnob = new knob( knobBright_26, this );
	m_pickKnob->move( 153, 142 );
	m_pickKnob->setHintText( tr( "Pick position:" ) + " ", "" );
	m_pickKnob->setWhatsThis( tr(
"The 'P' knob sets the position where the selected string will be 'picked'.  "
"The lower the setting the closer the pick is to the bridge." ) );

	m_pickupKnob = new knob( knobBright_26, this );
	m_pickupKnob->move( 177, 142 );
	m_pickupKnob->setHintText( tr( "Pickup position:" ) +
				" ", "" );
	m_pickupKnob->setWhatsThis( tr(
"The 'PU' knob sets the position where the vibrations will be monitored "
"for the selected string.  The lower the setting, the closer the "
"pickup is to the bridge." ) );

	m_panKnob = new knob( knobBright_26, this );
	m_panKnob->move( 105, 187 );
	m_panKnob->setHintText( tr( "Pan:" ) + " ", "" );
	m_panKnob->setWhatsThis( tr(
"The Pan knob determines the location of the selected string in the stereo "
"field." ) );
	
	m_detuneKnob = new knob( knobBright_26, this );
	m_detuneKnob->move( 150, 187 );
	m_detuneKnob->setHintText( tr( "Detune:" ) + " ", "" );
	m_detuneKnob->setWhatsThis( tr(
"The Detune knob modifies the pitch of the selected string.  Settings less "
"than zero will cause the string to sound flat.  Settings greater than zero "
"will cause the string to sound sharp." ) );

	m_randomKnob = new knob( knobBright_26, this );
	m_randomKnob->move( 194, 187 );
	m_randomKnob->setHintText( tr( "Fuzziness:" ) +
				" ", "" );
	m_randomKnob->setWhatsThis( tr(
"The Slap knob adds a bit of fuzz to the selected string which is most "
"apparent during the attack, though it can also be used to make the string "
"sound more 'metallic'.") );

	m_lengthKnob = new knob( knobBright_26, this );
	m_lengthKnob->move( 23, 193 );
	m_lengthKnob->setHintText( tr( "Length:" ) +
				" ", "" );
	m_lengthKnob->setWhatsThis( tr(
"The Length knob sets the length of the selected string.  Longer strings "
"will both ring longer and sound brighter, however, they will also eat up "
"more CPU cycles." ) );

	m_impulse = new ledCheckBox( "", this );
	m_impulse->move( 23, 94 );
	toolTip::add( m_impulse,
		      tr( "Impulse or initial state" ) );
	m_impulse->setWhatsThis( tr(
"The 'Imp' selector determines whether the waveform in the graph is to be "
"treated as an impulse imparted to the string by the pick or the initial "
"state of the string." ) );

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
	m_harmonic->setWhatsThis( tr(
"The Octave selector is used to choose which harmonic of the note the "
"string will ring at.  For example, '-2' means the string will ring two "
"octaves below the fundamental, 'F' means the string will ring at the "
"fundamental, and '6' means the string will ring six octaves above the "
"fundamental." ) );
	

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

	m_graph = new graph( this );
	m_graph->setWindowTitle( tr( "Impulse Editor" ) );
	m_graph->setForeground( PLUGIN_NAME::getIconPixmap( "wavegraph4" ) );
	m_graph->move( 76, 21 );
	m_graph->resize(132, 104);

	m_graph->setWhatsThis( tr(
"The waveform editor provides control over the initial state or impulse "
"that is used to start the string vibrating.  The buttons to the right of "
"the graph will initialize the waveform to the selected type.  The '?' "
"button will load a waveform from a file--only the first 128 samples "
"will be loaded.\n\n"

"The waveform can also be drawn in the graph.\n\n"

"The 'S' button will smooth the waveform.\n\n"

"The 'N' button will normalize the waveform.") );
	

	setWhatsThis( tr(
"Vibed models up to nine independently vibrating strings.  The 'String' "
"selector allows you to choose which string is being edited.  The 'Imp' " "selector chooses whether the graph represents an impulse or the initial "
"state of the string.  The 'Octave' selector chooses which harmonic the "
"string should vibrate at.\n\n"

"The graph allows you to control the initial state or impulse used to set the "
"string in motion.\n\n"

"The 'V' knob controls the volume.  The 'S' knob controls the string's "
"stiffness.  The 'P' knob controls the pick position.  The 'PU' knob "
"controls the pickup position.\n\n"

"'Pan' and 'Detune' hopefully don't need explanation.  The 'Slap' knob "
"adds a bit of fuzz to the sound of the string.\n\n"

"The 'Length' knob controls the length of the string.\n\n"

"The LED in the lower right corner of the waveform editor determines "
"whether the string is active in the current instrument." ) );


	m_power = new ledCheckBox( "", this, tr( "Enable waveform" ) );
	m_power->move( 212, 130 );
	toolTip::add( m_power,
			tr( "Click here to enable/disable waveform." ) );

	
	// String selector is not a part of the model
	m_stringSelector->setWindowTitle( tr( "String" ) );
	m_stringSelector->setWhatsThis( tr(
"The String selector is used to choose which string the controls are "
"editing.  A Vibed instrument can contain up to nine independently "
"vibrating strings.  The LED in the lower right corner of the "
"waveform editor indicates whether the selected string is active." ) );

	connect( m_stringSelector, SIGNAL( nineButtonSelection( int ) ),
			this, SLOT( showString( int ) ) );

	showString( 0 );

	m_sinWaveBtn = new pixmapButton( this, tr( "Sine wave" ) );
	m_sinWaveBtn->move( 212, 24 );
	m_sinWaveBtn->setActiveGraphic( embed::getIconPixmap(
				"sin_wave_active" ) );
	m_sinWaveBtn->setInactiveGraphic( embed::getIconPixmap(
				"sin_wave_inactive" ) );
	toolTip::add( m_sinWaveBtn,
				tr( "Use a sine-wave for "
				    "current oscillator." ) );
	connect( m_sinWaveBtn, SIGNAL (clicked () ),
			this, SLOT ( sinWaveClicked() ) );

	
	m_triangleWaveBtn = new pixmapButton( this, tr( "Triangle wave" ) );
	m_triangleWaveBtn->move( 212, 41 );
	m_triangleWaveBtn->setActiveGraphic(
			embed::getIconPixmap( "triangle_wave_active" ) );
	m_triangleWaveBtn->setInactiveGraphic(
			embed::getIconPixmap( "triangle_wave_inactive" ) );
	toolTip::add( m_triangleWaveBtn,
			tr( "Use a triangle-wave "
			    "for current oscillator." ) );
	connect( m_triangleWaveBtn, SIGNAL ( clicked () ),
			this, SLOT ( triangleWaveClicked( ) ) );

	
	m_sawWaveBtn = new pixmapButton( this, tr( "Saw wave" ) );
	m_sawWaveBtn->move( 212, 58 );
	m_sawWaveBtn->setActiveGraphic( embed::getIconPixmap(
				"saw_wave_active" ) );
	m_sawWaveBtn->setInactiveGraphic( embed::getIconPixmap(
				"saw_wave_inactive" ) );
	toolTip::add( m_sawWaveBtn,
				tr( "Use a saw-wave for "
				    "current oscillator." ) );
	connect( m_sawWaveBtn, SIGNAL (clicked () ),
			this, SLOT ( sawWaveClicked() ) );

	
	m_sqrWaveBtn = new pixmapButton( this, tr( "Square wave" ) );
	m_sqrWaveBtn->move( 212, 75 );
	m_sqrWaveBtn->setActiveGraphic( embed::getIconPixmap(
				"square_wave_active" ) );
	m_sqrWaveBtn->setInactiveGraphic( embed::getIconPixmap(
				"square_wave_inactive" ) );
	toolTip::add( m_sqrWaveBtn,
			tr( "Use a square-wave for "
			    "current oscillator." ) );
	connect( m_sqrWaveBtn, SIGNAL ( clicked () ),
			this, SLOT ( sqrWaveClicked() ) );

	
	m_whiteNoiseWaveBtn = new pixmapButton( this, tr( "White noise wave" ) );
	m_whiteNoiseWaveBtn->move( 212, 92 );
	m_whiteNoiseWaveBtn->setActiveGraphic(
			embed::getIconPixmap( "white_noise_wave_active" ) );
	m_whiteNoiseWaveBtn->setInactiveGraphic(
			embed::getIconPixmap( "white_noise_wave_inactive" ) );
	toolTip::add( m_whiteNoiseWaveBtn,
			tr( "Use white-noise for "
			    "current oscillator." ) );
	connect( m_whiteNoiseWaveBtn, SIGNAL ( clicked () ),
			this, SLOT ( noiseWaveClicked() ) );

	
	m_usrWaveBtn = new pixmapButton( this, tr( "User defined wave" ) );
	m_usrWaveBtn->move( 212, 109 );
	m_usrWaveBtn->setActiveGraphic( embed::getIconPixmap(
				"usr_wave_active" ) );
	m_usrWaveBtn->setInactiveGraphic( embed::getIconPixmap(
				"usr_wave_inactive" ) );
	toolTip::add( m_usrWaveBtn,
			tr( "Use a user-defined "
			    "waveform for current oscillator." ) );
	connect( m_usrWaveBtn, SIGNAL ( clicked () ),
			this, SLOT ( usrWaveClicked() ) );


	m_smoothBtn = new pixmapButton( this, tr( "Smooth" ) );
	m_smoothBtn->move( 79, 129 );
	m_smoothBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
			"smooth_active" ) );
	m_smoothBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
			"smooth_inactive" ) );
	m_smoothBtn->setChecked( false );
	toolTip::add( m_smoothBtn,
			tr( "Click here to smooth waveform." ) );
	connect( m_smoothBtn, SIGNAL ( clicked () ),
			this, SLOT ( smoothClicked() ) );
	
	m_normalizeBtn = new pixmapButton( this, tr( "Normalize" ) );
	m_normalizeBtn->move( 96, 129 );
	m_normalizeBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
			"normalize_active" ) );
	m_normalizeBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
			"normalize_inactive" ) );
	m_normalizeBtn->setChecked( false );
	toolTip::add( m_normalizeBtn,
			tr( "Click here to normalize waveform." ) );

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
	engine::getSong()->setModified();
}



void vibedView::triangleWaveClicked()
{
	m_graph->model()->setWaveToTriangle();
	engine::getSong()->setModified();
}



void vibedView::sawWaveClicked()
{
	m_graph->model()->setWaveToSaw();
	engine::getSong()->setModified();
}



void vibedView::sqrWaveClicked()
{
	m_graph->model()->setWaveToSquare();
	engine::getSong()->setModified();
}



void vibedView::noiseWaveClicked()
{
	m_graph->model()->setWaveToNoise();
	engine::getSong()->setModified();
}



void vibedView::usrWaveClicked()
{
	QString fileName = m_graph->model()->setWaveToUser();
	toolTip::add( m_usrWaveBtn, fileName );
	engine::getSong()->setModified();
}



void vibedView::smoothClicked()
{
	m_graph->model()->smooth();
	engine::getSong()->setModified();
}



void vibedView::normalizeClicked()
{
	m_graph->model()->normalize();
	engine::getSong()->setModified();
}




void vibedView::contextMenuEvent( QContextMenuEvent * )
{

	captionMenu contextMenu( model()->displayName(), this );
	contextMenu.addHelpAction();
	contextMenu.exec( QCursor::pos() );

}




void vibedView::displayHelp()
{
	QWhatsThis::showText( mapToGlobal( rect().bottomRight() ),
					whatsThis() );
}


extern "C"
{

// necessary for getting instance out of shared lib
Plugin * PLUGIN_EXPORT lmms_plugin_main( Model *, void * _data )
{
	return( new vibed( static_cast<InstrumentTrack *>( _data ) ) );
}


}

#include "moc_vibed.cxx"

