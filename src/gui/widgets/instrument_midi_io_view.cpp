#ifndef SINGLE_SOURCE_COMPILE

/*
 * instrument_midi_io_view.cpp - MIDI-IO-View
 *
 * Copyright (c) 2005-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include "midi_port_menu.h"
#include "embed.h"
#include "gui_templates.h"
#include "led_checkbox.h"
#include "lcd_spinbox.h"
#include "midi_client.h"
#include "tab_widget.h"
#include "tooltip.h"



instrumentMidiIOView::instrumentMidiIOView( QWidget * _parent ) :
	QWidget( _parent ),
	modelView( NULL, this ),
	m_rpBtn( NULL ),
	m_wpBtn( NULL )
{
	m_setupTabWidget = new tabWidget(
					tr( "MIDI SETUP FOR THIS INSTRUMENT" ),
									this );
	m_setupTabWidget->setGeometry( 4, 5, 238, 200 );

	m_inputChannelSpinBox = new lcdSpinBox( 3, m_setupTabWidget );
	m_inputChannelSpinBox->addTextForValue( 0, "---" );
	m_inputChannelSpinBox->setLabel( tr( "CHANNEL" ) );
	m_inputChannelSpinBox->move( 28, 52 );
	m_inputChannelSpinBox->setEnabled( FALSE );



	m_outputChannelSpinBox = new lcdSpinBox( 3, m_setupTabWidget );
	m_outputChannelSpinBox->setLabel( tr( "CHANNEL" ) );
	m_outputChannelSpinBox->move( 28, 132 );
	m_outputChannelSpinBox->setEnabled( FALSE );


	m_receiveCheckBox = new ledCheckBox( tr( "Receive MIDI-events" ),
				m_setupTabWidget );
	m_receiveCheckBox->move( 10, 34 );
	// enabling/disabling widgets is UI-stuff thus we do not use model here
	connect( m_receiveCheckBox, SIGNAL( toggled( bool ) ),
			m_inputChannelSpinBox, SLOT( setEnabled( bool ) ) );


	m_defaultVelocityInCheckBox = new ledCheckBox( tr( "Equal velocity" ),
							m_setupTabWidget );
	m_defaultVelocityInCheckBox->move( 28, 86 );


	m_sendCheckBox = new ledCheckBox( tr( "Send MIDI-events" ),
						m_setupTabWidget );
	m_sendCheckBox->move( 10, 114 );
	connect( m_sendCheckBox, SIGNAL( toggled( bool ) ),
			m_outputChannelSpinBox, SLOT( setEnabled( bool ) ) );


	m_defaultVelocityOutCheckBox = new ledCheckBox( tr( "Equal velocity" ),
							m_setupTabWidget );
	m_defaultVelocityOutCheckBox->move( 28, 166 );


	if( !engine::getMixer()->getMidiClient()->isRaw() )
	{
		m_rpBtn = new QToolButton( m_setupTabWidget );
		m_rpBtn->setText( tr( "MIDI-devices to receive "
						"MIDI-events from" ) );
		m_rpBtn->setIcon( embed::getIconPixmap( "midi_in" ) );
		m_rpBtn->setGeometry( 186, 34, 40, 40 );
		m_rpBtn->setPopupMode( QToolButton::InstantPopup );

		m_wpBtn = new QToolButton( m_setupTabWidget );
		m_wpBtn->setText( tr( "MIDI-devices to send MIDI-events "
								"to" ) );
		m_wpBtn->setIcon( embed::getIconPixmap( "midi_out" ) );
		m_wpBtn->setGeometry( 186, 114, 40, 40 );
		m_wpBtn->setPopupMode( QToolButton::InstantPopup );
	}
}




instrumentMidiIOView::~instrumentMidiIOView()
{
}




void instrumentMidiIOView::modelChanged( void )
{
	midiPort * mp = castModel<midiPort>();
	m_inputChannelSpinBox->setModel( &mp->m_inputChannelModel );
	m_outputChannelSpinBox->setModel( &mp->m_outputChannelModel );
	m_receiveCheckBox->setModel( &mp->m_readableModel );
	m_defaultVelocityInCheckBox->setModel(
				&mp->m_defaultVelocityInEnabledModel );
	m_sendCheckBox->setModel( &mp->m_writableModel );
	m_defaultVelocityOutCheckBox->setModel(
				&mp->m_defaultVelocityOutEnabledModel );
	if( m_rpBtn )
	{
		m_rpBtn->setMenu( mp->m_readablePortsMenu );
	}
	if( m_wpBtn )
	{
		m_wpBtn->setMenu( mp->m_writablePortsMenu );
	}
}


#endif
