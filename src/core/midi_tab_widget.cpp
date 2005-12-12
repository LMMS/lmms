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
#include <QListBox>
#include <QMenu>
#include <QToolButton>

#else

#include <qdom.h>
#include <qlistbox.h>
#include <qpopupmenu.h>
#include <qtoolbutton.h>

#endif


#include "midi_tab_widget.h"
#include "channel_track.h"
#include "midi_port.h"
#include "tab_widget.h"
#include "led_checkbox.h"
#include "lcd_spinbox.h"
#include "tooltip.h"
#include "song_editor.h"
#include "midi_client.h"
#include "embed.h"



midiTabWidget::midiTabWidget( channelTrack * _channel_track,
							midiPort * _port ) :
	QWidget( _channel_track->tabWidgetParent() ),
	settings(),
	m_channelTrack( _channel_track ),
	m_midiPort( _port ),
	m_readablePorts( NULL ),
	m_writeablePorts( NULL )
{
	m_setupTabWidget = new tabWidget( tr( "MIDI-SETUP FOR THIS CHANNEL" ),
									this );
	m_setupTabWidget->setGeometry( 4, 5, 238, 160 );


	m_inputChannelSpinBox = new lcdSpinBox( 0, MIDI_CHANNEL_COUNT, 3,
							m_setupTabWidget );
	m_inputChannelSpinBox->addTextForValue( 0, "---" );
	m_inputChannelSpinBox->setValue( m_midiPort->inputChannel() + 1 );
	m_inputChannelSpinBox->setLabel( tr( "CHANNEL" ) );
	m_inputChannelSpinBox->move( 28, 52 );
	connect( m_inputChannelSpinBox, SIGNAL( valueChanged( int ) ),
				this, SLOT( inputChannelChanged( int ) ) );

	m_outputChannelSpinBox = new lcdSpinBox( 0, MIDI_CHANNEL_COUNT, 3,
							m_setupTabWidget );
	m_outputChannelSpinBox->addTextForValue( 0, "---" );
	m_outputChannelSpinBox->setValue( m_midiPort->outputChannel() + 1 );
	m_outputChannelSpinBox->setLabel( tr( "CHANNEL" ) );
	m_outputChannelSpinBox->move( 28, 112 );
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
	m_sendCheckBox->move( 10, 94 );
	connect( m_sendCheckBox, SIGNAL( toggled( bool ) ),
				this, SLOT( midiPortModeToggled( bool ) ) );
	connect( m_sendCheckBox, SIGNAL( toggled( bool ) ),
			m_outputChannelSpinBox, SLOT( setEnabled( bool ) ) );


	midiPort::modes m = m_midiPort->mode();
	m_receiveCheckBox->setChecked( m == midiPort::INPUT ||
							m == midiPort::DUPLEX );
	m_sendCheckBox->setChecked( m == midiPort::OUTPUT ||
							m == midiPort::DUPLEX );

	midiClient * mc = mixer::inst()->getMIDIClient();
	// non-raw-clients have ports we can subscribe to!
	if( mc->isRaw() == FALSE )
	{
		m_readablePorts = new QMenu( m_setupTabWidget );
		m_readablePorts->setCheckable( TRUE );
		connect( m_readablePorts, SIGNAL( activated( int ) ),
				this, SLOT( activatedReadablePort( int ) ) );

		m_writeablePorts = new QMenu( m_setupTabWidget );
		m_writeablePorts->setCheckable( TRUE );
		connect( m_writeablePorts, SIGNAL( activated( int ) ),
				this, SLOT( activatedWriteablePort( int ) ) );

		// fill menus
		readablePortsChanged();
		writeablePortsChanged();

		QToolButton * rp_btn = new QToolButton( m_setupTabWidget );
		rp_btn->setTextLabel( tr( "MIDI-devices to receive "
						"MIDI-events from" ) );
		rp_btn->setIconSet( embed::getIconPixmap( "midi_in" ) );
		rp_btn->setGeometry( 186, 34, 40, 40 );
		rp_btn->setPopup( m_readablePorts );
		rp_btn->setPopupDelay( 1 );

		QToolButton * wp_btn = new QToolButton( m_setupTabWidget );
		wp_btn->setTextLabel( tr( "MIDI-devices to send MIDI-events "
								"to" ) );
		wp_btn->setPixmap( embed::getIconPixmap( "midi_out" ) );
		wp_btn->setGeometry( 186, 94, 40, 40 );
		wp_btn->setPopup( m_writeablePorts );
		wp_btn->setPopupDelay( 1 );

		// we want to get informed about port-changes!
		mc->connectRPChanged( this, SLOT( readablePortsChanged() ) );
		mc->connectWPChanged( this, SLOT( writeablePortsChanged() ) );
	}
	
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

	if( m_readablePorts != NULL && m_receiveCheckBox->isChecked() == TRUE )
	{
		QString rp;
		for( csize i = 0; i < m_readablePorts->count(); ++i )
		{
			int id = m_readablePorts->idAt( i );
			if( m_readablePorts->isItemChecked( id ) )
			{
				rp += m_readablePorts->text( id ) + ",";
			}
		}
		// cut off comma
		if( rp.length() > 0 )
		{
			rp.truncate( rp.length() - 1 );
		}
		mw_de.setAttribute( "inports", rp );
	}

	if( m_writeablePorts != NULL && m_sendCheckBox->isChecked() == TRUE )
	{
		QString wp;
		for( csize i = 0; i < m_writeablePorts->count(); ++i )
		{
			int id = m_writeablePorts->idAt( i );
			if( m_writeablePorts->isItemChecked( id ) )
			{
				wp += m_writeablePorts->text( id ) + ",";
			}
		}
		// cut off comma
		if( wp.length() > 0 )
		{
			wp.truncate( wp.length() - 1 );
		}
		mw_de.setAttribute( "outports", wp );
	}

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

	// restore connections

	QStringList rp = QStringList::split( ',', _this.attribute(
								"inports" ) );
	if( m_readablePorts != NULL && m_receiveCheckBox->isChecked() == TRUE )
	{
		for( csize i = 0; i < m_readablePorts->count(); ++i )
		{
			int id = m_readablePorts->idAt( i );
			if( m_readablePorts->isItemChecked( id ) !=
				( rp.find( m_readablePorts->text( id ) ) !=
								rp.end() ) )
			{
				activatedReadablePort( id );
			}
		}
	}

	QStringList wp = QStringList::split( ',', _this.attribute(
								"outports" ) );
	if( m_writeablePorts != NULL && m_sendCheckBox->isChecked() == TRUE )
	{
		for( csize i = 0; i < m_writeablePorts->count(); ++i )
		{
			int id = m_writeablePorts->idAt( i );
			if( m_writeablePorts->isItemChecked( id ) !=
				( wp.find( m_writeablePorts->text( id ) ) !=
								wp.end() ) )
			{
				activatedWriteablePort( id );
			}
		}
	}
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

	// check whether we have to dis-check items in connection-menu
	if( m_readablePorts != NULL && m_receiveCheckBox->isChecked() == FALSE )
	{
		for( csize i = 0; i < m_readablePorts->count(); ++i )
		{
			int id = m_readablePorts->idAt( i );
			if( m_readablePorts->isItemChecked( id ) )
			{
				activatedReadablePort( id );
			}
		}
	}
	if( m_writeablePorts != NULL && m_sendCheckBox->isChecked() == FALSE )
	{
		for( csize i = 0; i < m_writeablePorts->count(); ++i )
		{
			int id = m_writeablePorts->idAt( i );
			if( m_writeablePorts->isItemChecked( id ) )
			{
				activatedWriteablePort( id );
			}
		}
	}
	songEditor::inst()->setModified();
}




void midiTabWidget::readablePortsChanged( void )
{
	// first save all selected ports
	QStringList selected_ports;
	for( csize i = 0; i < m_readablePorts->count(); ++i )
	{
		int id = m_readablePorts->idAt( i );
		if( m_readablePorts->isItemChecked( id ) )
		{
			selected_ports.push_back( m_readablePorts->text( id ) );
		}
	}

	m_readablePorts->clear();
	const QStringList & rp = mixer::inst()->getMIDIClient()->
								readablePorts();
	// now insert new ports and restore selections
	for( QStringList::const_iterator it = rp.begin(); it != rp.end(); ++it )
	{
		int id = m_readablePorts->insertItem( *it );
		if( selected_ports.find( *it ) != selected_ports.end() )
		{
			m_readablePorts->setItemChecked( id, TRUE );
		}
	}
}




void midiTabWidget::writeablePortsChanged( void )
{
	// first save all selected ports
	QStringList selected_ports;
	for( csize i = 0; i < m_writeablePorts->count(); ++i )
	{
		int id = m_writeablePorts->idAt( i );
		if( m_writeablePorts->isItemChecked( id ) )
		{
			selected_ports.push_back( m_writeablePorts->text(
									id ) );
		}
	}

	m_writeablePorts->clear();
	const QStringList & wp = mixer::inst()->getMIDIClient()->
							writeablePorts();
	// now insert new ports and restore selections
	for( QStringList::const_iterator it = wp.begin(); it != wp.end(); ++it )
	{
		int id = m_writeablePorts->insertItem( *it );
		if( selected_ports.find( *it ) != selected_ports.end() )
		{
			m_writeablePorts->setItemChecked( id, TRUE );
		}
	}
}




void midiTabWidget::activatedReadablePort( int _id )
{
	// make sure, MIDI-port is configured for input
	if( m_readablePorts->isItemChecked( _id ) == FALSE &&
		m_midiPort->mode() != midiPort::INPUT &&
		m_midiPort->mode() != midiPort::DUPLEX )
	{
		m_receiveCheckBox->setChecked( TRUE );
	}
	m_readablePorts->setItemChecked( _id,
				!m_readablePorts->isItemChecked( _id ) );
	mixer::inst()->getMIDIClient()->subscribeReadablePort(
				m_midiPort, m_readablePorts->text( _id ),
				!m_readablePorts->isItemChecked( _id ) );
}




void midiTabWidget::activatedWriteablePort( int _id )
{
	// make sure, MIDI-port is configured for output
	if( m_writeablePorts->isItemChecked( _id ) == FALSE &&
		m_midiPort->mode() != midiPort::OUTPUT &&
		m_midiPort->mode() != midiPort::DUPLEX )
	{
		m_sendCheckBox->setChecked( TRUE );
	}
	m_writeablePorts->setItemChecked( _id,
				!m_writeablePorts->isItemChecked( _id ) );
	mixer::inst()->getMIDIClient()->subscribeWriteablePort(
				m_midiPort, m_writeablePorts->text( _id ),
				!m_writeablePorts->isItemChecked( _id ) );
}


#include "midi_tab_widget.moc"

