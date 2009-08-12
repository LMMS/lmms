/*
 * instrument_midi_io_view.cpp - MIDI-IO-View
 *
 * Copyright (c) 2005-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QtGui/QMenu>
#include <QtGui/QToolButton>

#include "instrument_midi_io_view.h"
#include "MidiPortMenu.h"
#include "engine.h"
#include "embed.h"
#include "group_box.h"
#include "gui_templates.h"
#include "lcd_spinbox.h"
#include "MidiClient.h"
#include "mixer.h"
#include "tooltip.h"



instrumentMidiIOView::instrumentMidiIOView( QWidget * _parent ) :
	QWidget( _parent ),
	modelView( NULL, this ),
	m_rpBtn( NULL ),
	m_wpBtn( NULL )
{
	m_midiInputGroupBox = new groupBox( tr( "ENABLE MIDI INPUT" ), this );
	m_midiInputGroupBox->setGeometry( 4, 5, 242, 80 );

	m_inputChannelSpinBox = new lcdSpinBox( 3, m_midiInputGroupBox );
	m_inputChannelSpinBox->addTextForValue( 0, "---" );
	m_inputChannelSpinBox->setLabel( tr( "CHANNEL" ) );
	m_inputChannelSpinBox->move( 16, 32 );
	m_inputChannelSpinBox->setEnabled( false );

	m_fixedInputVelocitySpinBox = new lcdSpinBox( 3, m_midiInputGroupBox );
	m_fixedInputVelocitySpinBox->addTextForValue( -1, "---" );
	m_fixedInputVelocitySpinBox->setLabel( tr( "VELOCITY" ) );
	m_fixedInputVelocitySpinBox->move( 64, 32 );
	m_fixedInputVelocitySpinBox->setEnabled( false );

	connect( m_midiInputGroupBox->ledButton(), SIGNAL( toggled( bool ) ),
			m_inputChannelSpinBox, SLOT( setEnabled( bool ) ) );
	connect( m_midiInputGroupBox->ledButton(), SIGNAL( toggled( bool ) ),
		m_fixedInputVelocitySpinBox, SLOT( setEnabled( bool ) ) );



	m_midiOutputGroupBox = new groupBox( tr( "ENABLE MIDI OUTPUT" ), this );
	m_midiOutputGroupBox->setGeometry( 4, 90, 242, 80 );

	m_outputChannelSpinBox = new lcdSpinBox( 3, m_midiOutputGroupBox );
	m_outputChannelSpinBox->setLabel( tr( "CHANNEL" ) );
	m_outputChannelSpinBox->move( 16, 32 );
	m_outputChannelSpinBox->setEnabled( false );

	m_fixedOutputVelocitySpinBox = new lcdSpinBox( 3, m_midiOutputGroupBox );
	m_fixedOutputVelocitySpinBox->addTextForValue( -1, "---" );
	m_fixedOutputVelocitySpinBox->setLabel( tr( "VELOCITY" ) );
	m_fixedOutputVelocitySpinBox->move( 64, 32 );
	m_fixedOutputVelocitySpinBox->setEnabled( false );

	m_outputProgramSpinBox = new lcdSpinBox( 3, m_midiOutputGroupBox );
	m_outputProgramSpinBox->setLabel( tr( "PROGRAM" ) );
	m_outputProgramSpinBox->move( 112, 32 );
	m_outputProgramSpinBox->setEnabled( false );

	connect( m_midiOutputGroupBox->ledButton(), SIGNAL( toggled( bool ) ),
			m_outputChannelSpinBox, SLOT( setEnabled( bool ) ) );
	connect( m_midiOutputGroupBox->ledButton(), SIGNAL( toggled( bool ) ),
		m_fixedOutputVelocitySpinBox, SLOT( setEnabled( bool ) ) );
	connect( m_midiOutputGroupBox->ledButton(), SIGNAL( toggled( bool ) ),
			m_outputProgramSpinBox, SLOT( setEnabled( bool ) ) );


	if( !engine::getMixer()->midiClient()->isRaw() )
	{
		m_rpBtn = new QToolButton( m_midiInputGroupBox );
		m_rpBtn->setText( tr( "MIDI devices to receive MIDI events from" ) );
		m_rpBtn->setIcon( embed::getIconPixmap( "piano" ) );
		m_rpBtn->setGeometry( 186, 24, 32, 32 );
		m_rpBtn->setPopupMode( QToolButton::InstantPopup );

		m_wpBtn = new QToolButton( m_midiOutputGroupBox );
		m_wpBtn->setText( tr( "MIDI devices to send MIDI events to" ) );
		m_wpBtn->setIcon( embed::getIconPixmap( "piano" ) );
		m_wpBtn->setGeometry( 186, 24, 32, 32 );
		m_wpBtn->setPopupMode( QToolButton::InstantPopup );
	}
}




instrumentMidiIOView::~instrumentMidiIOView()
{
}




void instrumentMidiIOView::modelChanged()
{
	MidiPort * mp = castModel<MidiPort>();

	m_midiInputGroupBox->setModel( &mp->m_readableModel );
	m_inputChannelSpinBox->setModel( &mp->m_inputChannelModel );
	m_fixedInputVelocitySpinBox->setModel( &mp->m_fixedInputVelocityModel );

	m_midiOutputGroupBox->setModel( &mp->m_writableModel );
	m_outputChannelSpinBox->setModel( &mp->m_outputChannelModel );
	m_fixedOutputVelocitySpinBox->setModel(
					&mp->m_fixedOutputVelocityModel );
	m_outputProgramSpinBox->setModel( &mp->m_outputProgramModel );

	if( m_rpBtn )
	{
		m_rpBtn->setMenu( mp->m_readablePortsMenu );
	}
	if( m_wpBtn )
	{
		m_wpBtn->setMenu( mp->m_writablePortsMenu );
	}
}


