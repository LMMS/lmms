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
#include "instrument_midi_io.h"
#include "embed.h"
#include "engine.h"
#include "gui_templates.h"
#include "midi_client.h"
#include "midi_port.h"
#include "mixer.h"
#include "led_checkbox.h"
#include "lcd_spinbox.h"
#include "tab_widget.h"
#include "tooltip.h"



instrumentMidiIOView::instrumentMidiIOView( QWidget * _parent ) :
	QWidget( _parent ),
	modelView( NULL ),
	m_readablePorts( NULL ),
	m_writeablePorts( NULL )
{
	m_setupTabWidget = new tabWidget( tr( "MIDI-SETUP FOR THIS CHANNEL" ),
									this );
	m_setupTabWidget->setGeometry( 4, 5, 238, 200 );

	m_inputChannelSpinBox = new lcdSpinBox( 3, m_setupTabWidget,
							tr( "Input channel" ) );
	m_inputChannelSpinBox->addTextForValue( 0, "---" );
	m_inputChannelSpinBox->setLabel( tr( "CHANNEL" ) );
	m_inputChannelSpinBox->move( 28, 52 );
	m_inputChannelSpinBox->setEnabled( FALSE );



	m_outputChannelSpinBox = new lcdSpinBox( 3, m_setupTabWidget,
						tr( "Output channel" ) );
	m_outputChannelSpinBox->setLabel( tr( "CHANNEL" ) );
	m_outputChannelSpinBox->move( 28, 132 );
	m_outputChannelSpinBox->setEnabled( FALSE );


	m_receiveCheckBox = new ledCheckBox( tr( "Receive MIDI-events" ),
						m_setupTabWidget,
						tr( "Receive MIDI-events" ) );
	m_receiveCheckBox->move( 10, 34 );
	// enabling/disabling widgets is UI-stuff thus we do not use model here
	connect( m_receiveCheckBox, SIGNAL( toggled( bool ) ),
			m_inputChannelSpinBox, SLOT( setEnabled( bool ) ) );


	m_defaultVelocityInCheckBox = new ledCheckBox(
				tr( "Default velocity for all input-events" ),
					m_setupTabWidget,
					tr( "Default input velocity" ) );
	m_defaultVelocityInCheckBox->move( 28, 84 );


	m_sendCheckBox = new ledCheckBox( tr( "Send MIDI-events" ),
						m_setupTabWidget,
						tr( "Send MIDI-events" ) );
	m_sendCheckBox->move( 10, 114 );
	connect( m_sendCheckBox, SIGNAL( toggled( bool ) ),
			m_outputChannelSpinBox, SLOT( setEnabled( bool ) ) );


	m_defaultVelocityOutCheckBox = new ledCheckBox(
				tr( "Default velocity for all output-events" ),
					m_setupTabWidget,
					tr( "Default output velocity" ) );
	m_defaultVelocityOutCheckBox->move( 28, 164 );



	// when using with non-raw-clients we can provide buttons showing
	// our port-menus when being clicked
	midiClient * mc = engine::getMixer()->getMIDIClient();
	if( mc->isRaw() == FALSE )
	{
		m_readablePorts = new QMenu( m_setupTabWidget );
		m_readablePorts->setFont( pointSize<9>(
						m_readablePorts->font() ) );
		connect( m_readablePorts, SIGNAL( triggered( QAction * ) ),
			this, SLOT( activatedReadablePort( QAction * ) ) );

		m_writeablePorts = new QMenu( m_setupTabWidget );
		m_writeablePorts->setFont( pointSize<9>(
						m_writeablePorts->font() ) );
		connect( m_writeablePorts, SIGNAL( triggered( QAction * ) ),
			this, SLOT( activatedWriteablePort( QAction * ) ) );

		// fill menus
/*		readablePortsChanged();
		writeablePortsChanged();*/

		QToolButton * rp_btn = new QToolButton( m_setupTabWidget );
		rp_btn->setText( tr( "MIDI-devices to receive "
						"MIDI-events from" ) );
		rp_btn->setIcon( embed::getIconPixmap( "midi_in" ) );
		rp_btn->setGeometry( 186, 34, 40, 40 );
		rp_btn->setMenu( m_readablePorts );
		rp_btn->setPopupMode( QToolButton::InstantPopup );

		QToolButton * wp_btn = new QToolButton( m_setupTabWidget );
		wp_btn->setText( tr( "MIDI-devices to send MIDI-events "
								"to" ) );
		wp_btn->setIcon( embed::getIconPixmap( "midi_out" ) );
		wp_btn->setGeometry( 186, 114, 40, 40 );
		wp_btn->setMenu( m_writeablePorts );
		wp_btn->setPopupMode( QToolButton::InstantPopup );
	}
}




instrumentMidiIOView::~instrumentMidiIOView()
{
}




void instrumentMidiIOView::modelChanged( void )
{
	instrumentMidiIO * mio = castModel<instrumentMidiIO>();
	m_inputChannelSpinBox->setModel( &mio->m_inputChannelModel );
	m_outputChannelSpinBox->setModel( &mio->m_outputChannelModel );
	m_receiveCheckBox->setModel( &mio->m_receiveEnabledModel );
	m_defaultVelocityInCheckBox->setModel(
				&mio->m_defaultVelocityInEnabledModel );
	m_sendCheckBox->setModel( &mio->m_sendEnabledModel );
	m_defaultVelocityOutCheckBox->setModel(
				&mio->m_defaultVelocityOutEnabledModel );
}




void instrumentMidiIOView::activatedReadablePort( QAction * _item )
{
	instrumentMidiIO * mio = castModel<instrumentMidiIO>();
	// make sure, MIDI-port is configured for input
	if( _item->isChecked() == TRUE &&
		mio->m_midiPort->mode() != midiPort::INPUT &&
		mio->m_midiPort->mode() != midiPort::DUPLEX )
	{
		mio->m_receiveEnabledModel.setValue( TRUE );
	}
	engine::getMixer()->getMIDIClient()->subscribeReadablePort(
			mio->m_midiPort, _item->text(), !_item->isChecked() );
}




void instrumentMidiIOView::activatedWriteablePort( QAction * _item )
{
	instrumentMidiIO * mio = castModel<instrumentMidiIO>();
	// make sure, MIDI-port is configured for output
	if( _item->isChecked() == TRUE &&
		mio->m_midiPort->mode() != midiPort::OUTPUT &&
		mio->m_midiPort->mode() != midiPort::DUPLEX )
	{
		mio->m_sendEnabledModel.setValue( TRUE );
	}
	engine::getMixer()->getMIDIClient()->subscribeWriteablePort(
			mio->m_midiPort, _item->text(), !_item->isChecked() );
}



#include "instrument_midi_io_view.moc"


#endif
