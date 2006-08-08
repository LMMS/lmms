#ifndef SINGLE_SOURCE_COMPILE

/*
 * effect_tab_widget.cpp - tab-widget in channel-track-window for setting up
 *                         effects
 *
 * Copyright (c) 2006 Danny McRae <khjklujn/at/users.sourceforge.net>
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

#include "ladspa_manager.h"
#ifdef LADSPA_SUPPORT

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
#include <qcolor.h>

#endif

#include "rack_plugin.h"
#include "knob.h"
#include "tooltip.h"
#include "ladspa_2_lmms.h"
#include "ladspa_control_dialog.h"
#include "audio_port.h"


rackPlugin::rackPlugin( QWidget * _parent, ladspa_key_t _key, instrumentTrack * _track, engine * _engine ) :
	QWidget( _parent, "rackPlugin" ),
	journallingObject( _engine )
{
	m_effect = new effect( _key, eng() );
	_track->getAudioPort()->getEffects()->appendEffect( m_effect );
	
	setFixedSize( 210, 58 );
	setPaletteBackgroundColor( QColor( 99, 99, 99 ) );
	
	m_grouper = new QGroupBox( 1, Qt::Vertical, "", this );
	m_grouper->setFixedSize( 210, 58 );
	
	m_bypass = new ledCheckBox( "", this, tr( "Turn the effect off" ), eng(), _track );
	connect( m_bypass, SIGNAL( toggled( bool ) ), this, SLOT( bypassed( bool ) ) );
	m_bypass->setChecked( TRUE );
	m_bypass->move( 3, 3 );
#ifdef QT4
		m_bypass->setWhatsThis(
#else
		QWhatsThis::add( m_bypass,
#endif
					tr( 
"Toggles the effect on or off." ) );
	
	m_wetDry = new knob( knobBright_26, this, tr( "Wet/Dry mix" ), eng(), _track );
	connect( m_wetDry, SIGNAL( valueChanged( float ) ), this, SLOT( setWetDry( float ) ) );
	m_wetDry->setLabel( tr( "W/D" ) );
	m_wetDry->setRange( 0.0f, 1.0f, 0.01f );
	m_wetDry->setInitValue( 1.0f );
	m_wetDry->move( 25, 3 );
	m_wetDry->setHintText( tr( "Wet Level:" ) + " ", "" );
#ifdef QT4
		m_wetDry->setWhatsThis(
#else
		QWhatsThis::add( m_wetDry,
#endif
					tr( 
"The wet/dry knob sets the ratio between the input signal and the effect that shows up in the output." ) );

	m_autoQuit = new knob( knobBright_26, this, tr( "Decay" ), eng(), _track );
	connect( m_autoQuit, SIGNAL( valueChanged( float ) ), this, SLOT( setAutoQuit( float ) ) );
	m_autoQuit->setLabel( tr( "Decay" ) );
	m_autoQuit->setRange( 1, 100, 1 );
	m_autoQuit->setInitValue( 1 );
	m_autoQuit->move( 60, 3 );
	m_autoQuit->setHintText( tr( "Buffers:" ) + " ", "" );
#ifdef QT4
		m_autoQuit->setWhatsThis(
#else
		QWhatsThis::add( m_autoQuit,
#endif
					tr( 
"The decay knob controls how many buffers of silence must pass before the plugin stops processing.  Smaller values "
"will reduce the CPU overhead but run the risk of clipping the tail on delay effects." ) );

	m_gate = new knob( knobBright_26, this, tr( "Decay" ), eng(), _track );
	connect( m_wetDry, SIGNAL( valueChanged( float ) ), this, SLOT( setGate( float ) ) );
	m_gate->setLabel( tr( "Gate" ) );
	m_gate->setRange( 0.0f, 1.0f, 0.01f );
	m_gate->setInitValue( 0.0f );
	m_gate->move( 95, 3 );
	m_gate->setHintText( tr( "Gate:" ) + " ", "" );
#ifdef QT4
		m_gate->setWhatsThis(
#else
		QWhatsThis::add( m_gate,
#endif
					tr( 
"The gate knob controls the signal level that is considered to be 'silence' while deciding when to stop processing "
"signals." ) );

	m_editButton = new QPushButton( this, "Controls" );
	m_editButton->setText( tr( "Controls" ) );
	m_editButton->setGeometry( 140, 19, 50, 20 );
	connect( m_editButton, SIGNAL( clicked() ), this, SLOT( editControls() ) );
		
	QString name = eng()->getLADSPAManager()->getShortName( _key );
	m_label = new QLabel( this );
	m_label->setText( name );
	QFont f = m_label->font();
	f.setBold( TRUE );
	m_label->setFont( pointSize<7>( f ) );
	m_label->setGeometry( 5, 38, 200, 20 );
	
	m_controlView = new ladspaControlDialog( this, m_effect, eng(), _track );
}



rackPlugin::~rackPlugin()
{
}



void rackPlugin::editControls( void )
{
	m_controlView->show();
	m_controlView->raise();
}




void rackPlugin::bypassed( bool _state )
{
	m_effect->setBypass( !_state );
}




void rackPlugin::setWetDry( float _value )
{
	m_effect->setWetDry( _value );
}



void rackPlugin::setAutoQuit( float _value )
{
	m_effect->setTimeout( static_cast<Uint32>( _value ) );
}



void rackPlugin::setGate( float _value )
{
	m_effect->setGate( _value );
}


#include "rack_plugin.moc"

#endif

#endif
