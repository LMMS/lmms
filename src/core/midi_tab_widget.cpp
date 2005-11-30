/*
 * midi_tab_widget.cpp - tab-widget in channel-track-window for setting up
 *                       MIDI-related stuff
 *
 * Copyright (c) 2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "qt3support.h"

#ifdef QT4

#include <Qt/QtXml>

#else

#include <qdom.h>

#endif


#include "midi_tab_widget.h"
#include "channel_track.h"
#include "midi_port.h"
#include "tab_widget.h"
#include "led_checkbox.h"
#include "lcd_spinbox.h"
#include "tooltip.h"
#include "song_editor.h"




midiTabWidget::midiTabWidget( channelTrack * _channel_track,
							midiPort * _port ) :
	QWidget( _channel_track->tabWidgetParent() ),
	settings(),
	m_channelTrack( _channel_track ),
	m_midiPort( _port )
{
	m_setupTabWidget = new tabWidget( tr( "MIDI-SETUP FOR THIS CHANNEL" ),
									this );
	m_setupTabWidget->setGeometry( 4, 5, 238, 130 );


	m_inputChannelSpinBox = new lcdSpinBox( 0, MIDI_CHANNEL_COUNT, 3,
							m_setupTabWidget );
	m_inputChannelSpinBox->addTextForValue( 0, "---" );
	m_inputChannelSpinBox->setValue( m_midiPort->inputChannel() + 1 );
	m_inputChannelSpinBox->setLabel( tr( "CHANNEL" ) );
	m_inputChannelSpinBox->move( 190, 30 );
	connect( m_inputChannelSpinBox, SIGNAL( valueChanged( int ) ),
				this, SLOT( inputChannelChanged( int ) ) );

	m_outputChannelSpinBox = new lcdSpinBox( 0, MIDI_CHANNEL_COUNT, 3,
							m_setupTabWidget );
	m_outputChannelSpinBox->addTextForValue( 0, "---" );
	m_outputChannelSpinBox->setValue( m_midiPort->outputChannel() + 1 );
	m_outputChannelSpinBox->setLabel( tr( "CHANNEL" ) );
	m_outputChannelSpinBox->move( 190, 60 );
	connect( m_outputChannelSpinBox, SIGNAL( valueChanged( int ) ),
				this, SLOT( outputChannelChanged( int ) ) );


	m_receiveCheckBox = new ledCheckBox( tr( "RECEIVE MIDI-EVENTS" ),
							m_setupTabWidget );
	m_receiveCheckBox->move( 10, 34 );
	connect( m_receiveCheckBox, SIGNAL( toggled( bool ) ),
				this, SLOT( midiPortModeToggled( bool ) ) );
	connect( m_receiveCheckBox, SIGNAL( toggled( bool ) ),
			m_inputChannelSpinBox, SLOT( setEnabled( bool ) ) );


	m_sendCheckBox = new ledCheckBox( tr( "SEND MIDI-EVENTS" ),
							m_setupTabWidget );
	m_sendCheckBox->move( 10, 64 );
	connect( m_sendCheckBox, SIGNAL( toggled( bool ) ),
				this, SLOT( midiPortModeToggled( bool ) ) );
	connect( m_sendCheckBox, SIGNAL( toggled( bool ) ),
			m_outputChannelSpinBox, SLOT( setEnabled( bool ) ) );


	m_routeCheckBox = new ledCheckBox( tr( "SEND RECEIVED MIDI-EVENTS" ),
							m_setupTabWidget );
	m_routeCheckBox->setChecked(
				m_channelTrack->midiEventRoutingEnabled() );
	m_routeCheckBox->move( 10, 100 );
	connect( m_sendCheckBox, SIGNAL( toggled( bool ) ),
		m_channelTrack, SLOT( toggleMidiEventRouting( bool ) ) );

	midiPort::modes m = m_midiPort->mode();
	m_receiveCheckBox->setChecked( m == midiPort::INPUT ||
							m == midiPort::DUPLEX );
	m_sendCheckBox->setChecked( m == midiPort::OUTPUT ||
							m == midiPort::DUPLEX );

}




midiTabWidget::~midiTabWidget()
{
}




void midiTabWidget::saveSettings( QDomDocument & _doc, QDomElement & _parent )
{
	QDomElement mw_de = _doc.createElement( nodeName() );
	mw_de.setAttribute( "inputchannel", QString::number(
					m_inputChannelSpinBox->value() ) );
	mw_de.setAttribute( "outputchannel", QString::number(
					m_outputChannelSpinBox->value() ) );
	mw_de.setAttribute( "receive", QString::number(
					m_receiveCheckBox->isChecked() ) );
	mw_de.setAttribute( "send", QString::number(
					m_sendCheckBox->isChecked() ) );
	mw_de.setAttribute( "route", QString::number(
					m_routeCheckBox->isChecked() ) );

	_parent.appendChild( mw_de );
}




void midiTabWidget::loadSettings( const QDomElement & _this )
{
	m_inputChannelSpinBox->setValue( _this.attribute( "inputchannel"
								).toInt() );
	m_outputChannelSpinBox->setValue( _this.attribute( "outputchannel"
								).toInt() );
	m_receiveCheckBox->setChecked( _this.attribute( "receive" ).toInt() );
	m_sendCheckBox->setChecked( _this.attribute( "send" ).toInt() );
	m_routeCheckBox->setChecked( _this.attribute( "route" ).toInt() );
}




void midiTabWidget::inputChannelChanged( int _new_chnl )
{
	m_midiPort->setInputChannel( _new_chnl - 1 );
	songEditor::inst()->setModified();
}




void midiTabWidget::outputChannelChanged( int _new_chnl )
{
	m_midiPort->setOutputChannel( _new_chnl - 1 );
	songEditor::inst()->setModified();
}




void midiTabWidget::midiPortModeToggled( bool )
{
	// this small lookup-table makes everything easier
	static const midiPort::modes modeTable[2][2] =
	{
		{ midiPort::DUMMY, midiPort::OUTPUT },
		{ midiPort::INPUT, midiPort::DUPLEX }
	} ;
	m_midiPort->setMode( modeTable[m_receiveCheckBox->isChecked()]
					[m_sendCheckBox->isChecked()] );

	songEditor::inst()->setModified();
}




#include "midi_tab_widget.moc"

