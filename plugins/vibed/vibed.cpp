/*
 * vibed.cpp - combination of PluckedStringSynth and BitInvader
 *
 * Copyright (c) 2006 Danny McRae <khjklujn/at/yahoo/com>
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

#include <Qt/QtXml>
#include <QtCore/QMap>
#include <QtGui/QLabel>
#include <QtGui/QMenu>
#include <QtGui/QWhatsThis>

#else

#include <qdom.h>
#include <qmap.h>
#include <qwhatsthis.h>
#include <qlabel.h>
#include <qpopupmenu.h>
#include <qcursor.h>

#endif


#include "vibed.h"
#include "note_play_handle.h"
#include "instrument_track.h"
#include "templates.h"
#include "buffer_allocator.h"
#include "knob.h"
#include "tooltip.h"
#include "oscillator.h"
#include "string_container.h"
#include "base64.h"

#undef SINGLE_SOURCE_COMPILE
#include "embed.cpp"
#include "volume_knob.h"
#include "volume.h"


extern "C"
{
	
plugin::descriptor vibedstrings_plugin_descriptor =
{
	STRINGIFY_PLUGIN_NAME( PLUGIN_NAME ),
	"Vibed",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"Vibrating string modeler" ),
	"Danny McRae <khjklujn/at/yahoo/com>",
	0x0100,
	plugin::INSTRUMENT,
	new QPixmap( PLUGIN_NAME::getIconPixmap( "logo" ) )
};

}


vibed::vibed( instrumentTrack * _channel_track ) :
		instrument( _channel_track, &vibedstrings_plugin_descriptor ),
	m_sampleLength( 128 )
{
#ifdef QT4
	setAutoFillBackground( TRUE );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap(
			"artwork" ) );
	setPalette( pal );
#else
	setErasePixmap( PLUGIN_NAME::getIconPixmap( "artwork" ) );
#endif

	for( Uint8 harm = 0; harm < 9; harm++ )
	{
		m_editor = new impulseEditor( this, 76, 21, eng(),
							_channel_track );
		m_editor->setOn( FALSE );
		m_editor->hide();
		m_editors.append( m_editor );
#ifdef QT4
	m_editor->setWhatsThis(
#else
	QWhatsThis::add( m_editor,
#endif
				tr( 
"The waveform editor provides control over the initial state or impulse "
"that is used to start the string vibrating.  The buttons to the right of "
"the graph will initialize the waveform to the selected type.  The '?' "
"button will load a waveform from a file--only the first 128 samples "
"will be loaded.\n\n"

"The waveform can also be drawn in the graph.\n\n"

"The 'S' button will smooth the waveform.\n\n"

"The 'N' button will normalize the waveform.") );

		m_volumeKnob = new volumeKnob( knobBright_26, this, 
					tr( "Volume" ), eng(), _channel_track );
		m_volumeKnob->setRange( MIN_VOLUME, MAX_VOLUME, 1.0f );
		m_volumeKnob->setInitValue( DEFAULT_VOLUME );
		m_volumeKnob->move( 103, 142 );
		m_volumeKnob->setHintText( tr( "Volume:" ) + " ", "" );
		m_volumeKnob->hide();
		m_volumeKnobs.append( m_volumeKnob );
#ifdef QT4
		m_volumeKnob->setWhatsThis(
#else
		QWhatsThis::add( m_volumeKnob,
#endif
					tr( 
"The 'V' knob sets the volume of the selected string." ) );
	
		m_stiffnessKnob = new knob( knobBright_26, this, 
							tr( "String stiffness" ),
							eng(), _channel_track );
		m_stiffnessKnob->setRange( 0.0f, 0.05f, 0.001f );
		m_stiffnessKnob->setInitValue( 0.0f );
		m_stiffnessKnob->move( 129, 142 );
		m_stiffnessKnob->setHintText( tr( "String stiffness:" ) + 
						" ", "" );
		m_stiffnessKnob->hide();
		m_stiffnessKnobs.append( m_stiffnessKnob );
#ifdef QT4
		m_stiffnessKnob->setWhatsThis(
#else
		QWhatsThis::add( m_stiffnessKnob,
#endif
					tr( 
"The 'S' knob sets the stiffness of the selected string.  The stiffness "
"of the string affects how long the string will ring out.  The lower "
"the setting, the longer the string will ring." ) );
		
		
		m_pickKnob = new knob( knobBright_26, this, 
							tr( "Pick position" ),
							eng(), _channel_track );
		m_pickKnob->setRange( 0.0f, 0.5f, 0.005f );
		m_pickKnob->setInitValue( 0.0f );
		m_pickKnob->move( 153, 142 );
		m_pickKnob->setHintText( tr( "Pick position:" ) + " ", "" );
		m_pickKnob->hide();
		m_pickKnobs.append( m_pickKnob );
#ifdef QT4
		m_pickKnob->setWhatsThis(
#else
		QWhatsThis::add( m_pickKnob,
#endif
					tr( 
"The 'P' knob sets the position where the selected string will be 'picked'.  "
"The lower the setting the closer the pick is to the bridge." ) );
	
		m_pickupKnob = new knob( knobBright_26, this, 
							tr( "Pickup position" ),
							eng(), _channel_track );
		m_pickupKnob->setRange( 0.0f, 0.5f, 0.005f );
		m_pickupKnob->setInitValue( 0.05f );
		m_pickupKnob->move( 177, 142 );
		m_pickupKnob->setHintText( tr( "Pickup position:" ) + 
					" ", "" );
		m_pickupKnob->hide();
		m_pickupKnobs.append( m_pickupKnob );
#ifdef QT4
		m_pickupKnob->setWhatsThis(
#else
		QWhatsThis::add( m_pickupKnob,
#endif
					tr( 
"The 'PU' knob sets the position where the vibrations will be monitored "
"for the selected string.  The lower the setting, the closer the "
"pickup is to the bridge." ) );

 		m_panKnob = new knob( knobBright_26, this, 
 					tr( "Pan" ), eng(), _channel_track );
	 	m_panKnob->setRange( -1.0f, 1.0f, 0.01f );
 		m_panKnob->setInitValue( 0.0f );
	 	m_panKnob->move( 105, 187 );
 		m_panKnob->setHintText( tr( "Pan:" ) + " ", "" );
		m_panKnob->hide();
		m_panKnobs.append( m_panKnob );
#ifdef QT4
		m_panKnob->setWhatsThis(
#else
		QWhatsThis::add( m_panKnob,
#endif
					tr( 
"The Pan knob determines the location of the selected string in the stereo "
"field." ) );
		
	 	m_detuneKnob = new knob( knobBright_26, this, 
							tr( "Detune" ),
		 					eng(), _channel_track );
 		m_detuneKnob->setRange( -0.1f, 0.1f, 0.001f );
 		m_detuneKnob->setInitValue( 0.0f );
 		m_detuneKnob->move( 150, 187 );
 		m_detuneKnob->setHintText( tr( "Detune:" ) + " ", "" );
		m_detuneKnob->hide();
		m_detuneKnobs.append( m_detuneKnob );
#ifdef QT4
		m_detuneKnob->setWhatsThis(
#else
		QWhatsThis::add( m_detuneKnob,
#endif
					tr( 
"The Detune knob modifies the pitch of the selected string.  Settings less "
"than zero will cause the string to sound flat.  Settings greater than zero "
"will cause the string to sound sharp." ) );
	
		m_randomKnob = new knob( knobBright_26, this, 
							tr( "Fuzziness" ),
							eng(), _channel_track );
		m_randomKnob->setRange( 0.0f, 0.75f, 0.01f );
		m_randomKnob->setInitValue( 0.0f );
		m_randomKnob->move( 194, 187 );
		m_randomKnob->setHintText( tr( "Fuzziness:" ) + 
					" ", "" );
		m_randomKnob->hide();
		m_randomKnobs.append( m_randomKnob );
#ifdef QT4
		m_randomKnob->setWhatsThis(
#else
		QWhatsThis::add( m_randomKnob,
#endif
					tr( 
"The Slap knob adds a bit of fuzz to the selected string which is most "
"apparent during the attack, though it can also be used to make the string "
"sound more 'metallic'.") );

		m_lengthKnob = new knob( knobBright_26, this, 
					tr( "Length" ), eng(), _channel_track );
		m_lengthKnob->setRange( 1, 16, 1 );
		m_lengthKnob->setInitValue( 1 );
		m_lengthKnob->move( 23, 193 );
		m_lengthKnob->setHintText( tr( "Length:" ) + 
					" ", "" );
		m_lengthKnob->hide();
		m_lengthKnobs.append( m_lengthKnob );
#ifdef QT4
		m_lengthKnob->setWhatsThis(
#else
		QWhatsThis::add( m_lengthKnob,
#endif
					tr( 
"The Length knob sets the length of the selected string.  Longer strings "
"will both ring longer and sound brighter, however, they will also eat up "
"more CPU cycles." ) );

		m_impulse = new ledCheckBox( "", this, tr( "Impulse" ), eng(),
							_channel_track );
		m_impulse->move( 23, 94 );
		m_impulse->setChecked( FALSE );
		toolTip::add( m_impulse,
			      tr( "Impulse or initial state" ) );
		m_impulse->hide();
		m_impulses.append( m_impulse );
#ifdef QT4
		m_impulse->setWhatsThis(
#else
		QWhatsThis::add( m_impulse,
#endif
					tr( 
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
			this,
			eng(),
			NULL );
		m_harmonic->hide();
		m_harmonics.append( m_harmonic );
#ifdef QT4
		m_harmonic->setWhatsThis(
#else
		QWhatsThis::add( m_harmonic,
#endif
					tr( 
"The Octave selector is used to choose which harmonic of the note the "
"string will ring at.  For example, '-2' means the string will ring two "
"octaves below the fundamental, 'F' means the string will ring at the "
"fundamental, and '6' means the string will ring six octaves above the "
"fundamental." ) );
	}
	
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
			this,
			eng(),
			NULL );
	connect( m_stringSelector, SIGNAL( nineButtonSelection( Uint8 ) ),
			this, SLOT( showString( Uint8 ) ) );
#ifdef QT4
		m_stringSelector->setWhatsThis(
#else
		QWhatsThis::add( m_stringSelector,
#endif
					tr( 
"The String selector is used to choose which string the controls are "
"editting.  A Vibed instrument can contain up to nine independently "
"vibrating strings.  The LED in the lower right corner of the "
"waveform editor indicates whether the selected string is active." ) );

	m_pickKnob = m_pickKnobs[0];
	m_pickupKnob = m_pickupKnobs[0];
	m_stiffnessKnob = m_stiffnessKnobs[0];
	m_volumeKnob = m_volumeKnobs[0];
	m_panKnob = m_panKnobs[0];
	m_detuneKnob = m_detuneKnobs[0];
	m_randomKnob = m_randomKnobs[0];
	m_lengthKnob = m_lengthKnobs[0];
	m_editor = m_editors[0];
	m_impulse = m_impulses[0];
	m_harmonic = m_harmonics[0];
	
	m_editor->setOn( TRUE );
	showString( 0 );

#ifdef QT4
	this->setWhatsThis(
#else
	QWhatsThis::add( this,
#endif
				tr( 
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

}




vibed::~vibed()
{
}




void vibed::saveSettings( QDomDocument & _doc,
				QDomElement & _this )
{
	QString name;
	
	// Save plugin version
	_this.setAttribute( "version", "0.1" );
	
	for( Uint8 i = 0; i < 9; i++ )
	{
		name = "active" + QString::number( i );
		_this.setAttribute( name, QString::number( 
					m_editors[i]->isOn() ) );
		if( m_editors[i]->isOn() )
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
			_this.setAttribute( name, QString::number( 
					m_harmonics[i]->getSelected() ) );

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
				(const char *)m_editors[i]->getValues(), 
				 128 * sizeof(float), sampleString );
			name = "graph" + QString::number( i );
			_this.setAttribute( name, sampleString );
		}
	}

}




void vibed::loadSettings( const QDomElement & _this )
{
	QString name;
	
	for( Uint8 i = 0; i < 9; i++ )
	{
		name = "active" + QString::number( i );
		m_editors[i]->setOn( _this.attribute( name ).toInt() );
		
		if( m_editors[i]->isOn() )
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
			m_harmonics[i]->setSelected( 
					_this.attribute( name ).toInt() );
			
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
						(char * *) &shp, &size );
			// TODO: check whether size == 128 * sizeof( float ),
			// otherwise me might and up in a segfault
			m_editors[i]->setValues( shp );
			delete[] shp;

			// TODO: do one of the following to avoid
			// "uninitialized" wave-shape-buttongroup
			// - activate random-wave-shape-button here
			// - make wave-shape-buttons simple toggle-buttons
			//   instead of checkable buttons
			// - save and restore selected wave-shape-button
		}
	}
	
	update();
}




QString vibed::nodeName( void ) const
{
	return( vibedstrings_plugin_descriptor.name );
}




void vibed::playNote( notePlayHandle * _n )
{
	if ( _n->totalFramesPlayed() == 0 )
	{
		float freq = getInstrumentTrack()->frequency( _n );
		
		_n->m_pluginData = new stringContainer( 
				freq,
				eng()->getMixer()->sampleRate(),
				m_sampleLength );
		
		for( Uint8 i = 0; i < 9; ++i )
		{
			if( m_editors[i]->isOn() )
			{
				static_cast<stringContainer*>(
					_n->m_pluginData )->addString(
				m_harmonics[i]->getSelected(),
				m_pickKnobs[i]->value(),
				m_pickupKnobs[i]->value(),
				m_editors[i]->getValues(),
				m_randomKnobs[i]->value(),
				m_stiffnessKnobs[i]->value(),
				m_detuneKnobs[i]->value(),
				static_cast<int>(
					m_lengthKnobs[i]->value() ),
				m_impulses[i]->isChecked(),
				i );
			}
		}
	}

	const fpab_t frames = eng()->getMixer()->framesPerAudioBuffer();
	stringContainer * ps = static_cast<stringContainer *>(
			 				_n->m_pluginData );
	
	sampleFrame * buf = bufferAllocator::alloc<sampleFrame>( frames );

	for( fpab_t i = 0; i < frames; ++i )
	{
		buf[i][0] = 0.0f;
		buf[i][1] = 0.0f;
		Uint8 s = 0;
		for( Uint8 string = 0; string < 9; ++string )
		{
			if( ps->exists( string ) )
			{
				// pan: 0 -> left, 1 -> right
				const float pan = ( 
					m_panKnobs[string]->value() + 1 ) /
									2.0f;
				const sample_t sample = ps->getStringSample(
									s ) *
					m_volumeKnobs[string]->value() / 100.0f;
				buf[i][0] += ( 1.0f - pan ) * sample;
				buf[i][1] += pan * sample;
				s++;
			}
		}
	}

	getInstrumentTrack()->processAudioBuffer( buf, frames, _n );
	
	bufferAllocator::free( buf );
}




void vibed::deleteNotePluginData( notePlayHandle * _n )
{
	delete static_cast<stringContainer *>( _n->m_pluginData );
}




void vibed::showString( Uint8 _string )
{
	m_pickKnob->hide();
	m_pickupKnob->hide();
	m_stiffnessKnob->hide();
	m_volumeKnob->hide();
	m_panKnob->hide();
	m_detuneKnob->hide();
	m_randomKnob->hide();
	m_lengthKnob->hide();
	m_editor->hide();
	m_impulse->hide();
	m_harmonic->hide();
	
	// TODO: first assign, then show - avoids that we have to index vector
	// (or list or whatever) twice
	// something like
	// ( m_editor = m_editors[_string] )->show()
	// would be even better ;-)
	m_editors[_string]->show();
	m_volumeKnobs[_string]->show();
	m_stiffnessKnobs[_string]->show();
	m_pickKnobs[_string]->show();
	m_pickupKnobs[_string]->show();
	m_panKnobs[_string]->show();
	m_detuneKnobs[_string]->show();
	m_randomKnobs[_string]->show();
	m_lengthKnobs[_string]->show();
	m_impulses[_string]->show();
	m_impulses[_string]->update();
	m_harmonics[_string]->show();
	
	m_pickKnob = m_pickKnobs[_string];
	m_pickupKnob = m_pickupKnobs[_string];
	m_stiffnessKnob = m_stiffnessKnobs[_string];
	m_volumeKnob = m_volumeKnobs[_string];
	m_panKnob = m_panKnobs[_string];
	m_detuneKnob = m_detuneKnobs[_string];
	m_randomKnob = m_randomKnobs[_string];
	m_lengthKnob = m_lengthKnobs[_string];
	m_editor = m_editors[_string];
	m_impulse = m_impulses[_string];
	m_harmonic = m_harmonics[_string];
}





void vibed::contextMenuEvent( QContextMenuEvent * )
{
	QMenu contextMenu( this );
#ifdef QT4
	contextMenu.setTitle( accessibleName() );
#else
	QLabel * caption = new QLabel( "<font color=white><b>" +
			QString( "Vibed" ) + "</b></font>", this );
	caption->setPaletteBackgroundColor( QColor( 0, 0, 192 ) );
	caption->setAlignment( Qt::AlignCenter );
	contextMenu.addAction( caption );
#endif
	contextMenu.addAction( embed::getIconPixmap( "help" ), tr( "&Help" ),
						this, SLOT( displayHelp() ) );
	contextMenu.exec( QCursor::pos() );
}




void vibed::displayHelp( void )
{
#ifdef QT4
	QWhatsThis::showText( mapToGlobal( rect().bottomRight() ),
			      whatsThis() );
#else
	QWhatsThis::display( QWhatsThis::textFor( this ), mapToGlobal(
						rect().bottomRight() ) );
#endif
}




extern "C"
{

// neccessary for getting instance out of shared lib
	plugin * lmms_plugin_main( void * _data )
	{
		return( new vibed( static_cast<instrumentTrack *>( _data ) ) );
	}


}

#include "vibed.moc"

