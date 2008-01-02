#ifndef SINGLE_SOURCE_COMPILE

/*
 * midi_tab_widget.cpp - tab-widget in channel-track-window for setting up
 *                       MIDI-related stuff
 *
 * Copyright (c) 2005-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <Qt/QtXml>
#include <QtGui/QMenu>
#include <QtGui/QToolButton>


#include "midi_tab_widget.h"
#include "embed.h"
#include "engine.h"
#include "gui_templates.h"
#include "instrument_track.h"
#include "midi_client.h"
#include "midi_port.h"
#include "led_checkbox.h"
#include "lcd_spinbox.h"
#include "song_editor.h"
#include "tab_widget.h"
#include "tooltip.h"
#include "automatable_model_templates.h"



midiTabWidget::midiTabWidget( instrumentTrack * _instrument_track,
							midiPort * _port ) :
	QWidget( _instrument_track->tabWidgetParent() ),
	m_instrumentTrack( _instrument_track ),
	m_midiPort( _port ),
        m_inputChannelModel(),
        m_outputChannelModel(),
        m_receiveEnabledModel( FALSE, FALSE, TRUE ),
        m_sendEnabledModel( FALSE, FALSE, TRUE ),
        m_defaultVelocityInEnabledModel( FALSE, FALSE, TRUE ),
        m_defaultVelocityOutEnabledModel( FALSE, FALSE, TRUE ),
	m_readablePorts( NULL ),
	m_writeablePorts( NULL )
{
	m_setupTabWidget = new tabWidget( tr( "MIDI-SETUP FOR THIS CHANNEL" ),
									this );
	m_setupTabWidget->setGeometry( 4, 5, 238, 200 );

	m_inputChannelModel.setTrack( m_instrumentTrack );
	m_inputChannelModel.setRange( 0, MIDI_CHANNEL_COUNT );
	m_inputChannelModel.setValue( m_midiPort->inputChannel() + 1 );
	connect( &m_inputChannelModel, SIGNAL( dataChanged() ),
				this, SLOT( inputChannelChanged() ) );

	m_inputChannelSpinBox = new lcdSpinBox( 3, m_setupTabWidget,
							tr( "Input channel" ) );
	m_inputChannelSpinBox->setModel( &m_inputChannelModel );
	m_inputChannelSpinBox->addTextForValue( 0, "---" );
	m_inputChannelSpinBox->setLabel( tr( "CHANNEL" ) );
	m_inputChannelSpinBox->move( 28, 52 );
	m_inputChannelSpinBox->setEnabled( FALSE );
	inputChannelChanged();



	m_outputChannelModel.setTrack( m_instrumentTrack );
	m_outputChannelModel.setRange( 1, MIDI_CHANNEL_COUNT );
	m_outputChannelModel.setValue( m_midiPort->outputChannel() + 1 );
	connect( &m_outputChannelModel, SIGNAL( dataChanged() ),
				this, SLOT( outputChannelChanged() ) );

	m_outputChannelSpinBox = new lcdSpinBox( 3, m_setupTabWidget,
						tr( "Output channel" ) );
	m_outputChannelSpinBox->setLabel( tr( "CHANNEL" ) );
	m_outputChannelSpinBox->move( 28, 132 );
	m_outputChannelSpinBox->setEnabled( FALSE );
	outputChannelChanged();


	m_receiveEnabledModel.setTrack( m_instrumentTrack );
	m_receiveCheckBox = new ledCheckBox( tr( "Receive MIDI-events" ),
						m_setupTabWidget,
						tr( "Receive MIDI-events" ) );
	m_receiveCheckBox->setModel( &m_receiveEnabledModel );
	m_receiveCheckBox->move( 10, 34 );
	// enabling/disabling widgets is UI-stuff thus we do not use model here
	connect( m_receiveCheckBox, SIGNAL( toggled( bool ) ),
			m_inputChannelSpinBox, SLOT( setEnabled( bool ) ) );
	connect( &m_receiveEnabledModel, SIGNAL( dataChanged() ),
					this, SLOT( midiPortModeChanged() ) );


	m_defaultVelocityInEnabledModel.setTrack( m_instrumentTrack );
	m_defaultVelocityInCheckBox = new ledCheckBox( tr( "Default velocity "
						"for all input-events" ),
						m_setupTabWidget,
						tr( "Default input velocity" ) );
	m_defaultVelocityInCheckBox->setModel(
					&m_defaultVelocityInEnabledModel );
	m_defaultVelocityInCheckBox->move( 28, 84 );
	connect( &m_defaultVelocityInEnabledModel, SIGNAL( dataChanged() ),
					this, SLOT( defaultVelInChanged() ) );


	m_sendEnabledModel.setTrack( m_instrumentTrack );
	m_sendCheckBox = new ledCheckBox( tr( "Send MIDI-events" ),
						m_setupTabWidget,
						tr( "Send MIDI-events" ) );
	m_sendCheckBox->setModel( &m_sendEnabledModel );
	m_sendCheckBox->move( 10, 114 );
	connect( m_sendCheckBox, SIGNAL( toggled( bool ) ),
			m_outputChannelSpinBox, SLOT( setEnabled( bool ) ) );
	connect( &m_sendEnabledModel, SIGNAL( dataChanged() ),
					this, SLOT( midiPortModeChanged() ) );


	m_defaultVelocityOutEnabledModel.setTrack( m_instrumentTrack );
	m_defaultVelocityOutCheckBox = new ledCheckBox( tr( "Default velocity "
						"for all output-events" ),
						m_setupTabWidget,
						tr( "Default output velocity" ) );
	m_defaultVelocityOutCheckBox->setModel(
					&m_defaultVelocityOutEnabledModel );
	m_defaultVelocityOutCheckBox->move( 28, 164 );
	connect( &m_defaultVelocityOutEnabledModel, SIGNAL( dataChanged() ),
				this, SLOT( defaultVelOutChanged() ) );



	const midiPort::modes m = m_midiPort->mode();
	m_receiveEnabledModel.setValue( m == midiPort::INPUT ||
							m == midiPort::DUPLEX );
	m_sendEnabledModel.setValue( m == midiPort::OUTPUT ||
							m == midiPort::DUPLEX );

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
		readablePortsChanged();
		writeablePortsChanged();

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

		// we want to get informed about port-changes!
		mc->connectRPChanged( this, SLOT( readablePortsChanged() ) );
		mc->connectWPChanged( this, SLOT( writeablePortsChanged() ) );
	}
}




midiTabWidget::~midiTabWidget()
{
}




void midiTabWidget::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	m_inputChannelModel.saveSettings( _doc, _this, "inputchannel" );
	m_outputChannelModel.saveSettings( _doc, _this, "outputchannel" );
	m_receiveEnabledModel.saveSettings( _doc, _this, "receive" );
	m_sendEnabledModel.saveSettings( _doc, _this, "send" );
	m_defaultVelocityInEnabledModel.saveSettings( _doc, _this, "defvelin" );
	m_defaultVelocityOutEnabledModel.saveSettings( _doc, _this, "defvelout" );

	if( m_readablePorts != NULL && m_receiveEnabledModel.value() == TRUE )
	{
		// TODO: M/V-split!
		QString rp;
		QList<QAction *> actions = m_readablePorts->actions();
		for( QList<QAction *>::iterator it = actions.begin();
						it != actions.end(); ++it )
		{
			if( ( *it )->isChecked() )
			{
				rp += ( *it )->text() + ",";
			}
		}
		// cut off comma
		if( rp.length() > 0 )
		{
			rp.truncate( rp.length() - 1 );
		}
		_this.setAttribute( "inports", rp );
	}

	if( m_writeablePorts != NULL && m_sendEnabledModel.value() == TRUE )
	{
		// TODO: M/V-split!
		QString wp;
		QList<QAction *> actions = m_writeablePorts->actions();
		for( QList<QAction *>::iterator it = actions.begin();
						it != actions.end(); ++it )
		{
			if( ( *it )->isChecked() )
			{
				wp += ( *it )->text() + ",";
			}
		}
		// cut off comma
		if( wp.length() > 0 )
		{
			wp.truncate( wp.length() - 1 );
		}
		_this.setAttribute( "outports", wp );
	}
}




void midiTabWidget::loadSettings( const QDomElement & _this )
{
	m_inputChannelModel.loadSettings( _this, "inputchannel" );
	m_outputChannelModel.loadSettings( _this, "outputchannel" );
	m_receiveEnabledModel.loadSettings( _this, "receive" );
	m_sendEnabledModel.loadSettings( _this, "send" );
	m_defaultVelocityInEnabledModel.loadSettings( _this, "defvelin" );
	m_defaultVelocityOutEnabledModel.loadSettings( _this, "defvelout" );

	// restore connections

	if( m_readablePorts != NULL && m_receiveEnabledModel.value() == TRUE )
	{
		// TODO: M/V-split!
		QStringList rp = _this.attribute( "inports" ).split( ',' );
		QList<QAction *> actions = m_readablePorts->actions();
		for( QList<QAction *>::iterator it = actions.begin();
						it != actions.end(); ++it )
		{
			if( ( *it )->isChecked() !=
				( rp.indexOf( ( *it )->text() ) != -1 ) )
			{
				( *it )->setChecked( TRUE );
				activatedReadablePort( *it );
			}
		}
	}

	if( m_writeablePorts != NULL && m_sendEnabledModel.value() == TRUE )
	{
		// TODO: M/V-split!
		QStringList wp = _this.attribute( "outports" ).split( ',' );
		QList<QAction *> actions = m_writeablePorts->actions();
		for( QList<QAction *>::iterator it = actions.begin();
						it != actions.end(); ++it )
		{
			if( ( *it )->isChecked() !=
				( wp.indexOf( ( *it )->text() ) != -1 ) )
			{
				( *it )->setChecked( TRUE );
				activatedWriteablePort( *it );
			}
		}
	}
}




void midiTabWidget::inputChannelChanged( void )
{
	m_midiPort->setInputChannel( m_inputChannelModel.value() - 1 );
	engine::getSongEditor()->setModified();
}




void midiTabWidget::outputChannelChanged( void )
{
	m_midiPort->setOutputChannel( m_outputChannelModel.value() - 1 );
	engine::getSongEditor()->setModified();
}




void midiTabWidget::defaultVelInChanged( void )
{
	m_midiPort->enableDefaultVelocityForInEvents(
				m_defaultVelocityInEnabledModel.value() );
}




void midiTabWidget::defaultVelOutChanged( void )
{
	m_midiPort->enableDefaultVelocityForOutEvents(
				m_defaultVelocityOutEnabledModel.value() );
}




void midiTabWidget::midiPortModeChanged( void )
{
	// this small lookup-table makes everything easier
	static const midiPort::modes modeTable[2][2] =
	{
		{ midiPort::DUMMY, midiPort::OUTPUT },
		{ midiPort::INPUT, midiPort::DUPLEX }
	} ;
	m_midiPort->setMode( modeTable[m_receiveEnabledModel.value()]
					[m_sendEnabledModel.value()] );

	// check whether we have to dis-check items in connection-menu
	if( m_readablePorts != NULL && m_receiveEnabledModel.value() == FALSE )
	{
		QList<QAction *> actions = m_readablePorts->actions();
		for( QList<QAction *>::iterator it = actions.begin();
						it != actions.end(); ++it )
		{
			if( ( *it )->isChecked() == TRUE )
			{
				( *it )->setChecked( FALSE );
				activatedReadablePort( *it );
			}
		}
	}

	if( m_writeablePorts != NULL && m_sendEnabledModel.value() == FALSE )
	{
		QList<QAction *> actions = m_writeablePorts->actions();
		for( QList<QAction *>::iterator it = actions.begin();
						it != actions.end(); ++it )
		{
			if( ( *it )->isChecked() == TRUE )
			{
				( *it )->setChecked( FALSE );
				activatedWriteablePort( *it );
			}
		}
	}
	engine::getSongEditor()->setModified();
}




void midiTabWidget::readablePortsChanged( void )
{
	// first save all selected ports
	QStringList selected_ports;
	QList<QAction *> actions = m_readablePorts->actions();
	for( QList<QAction *>::iterator it = actions.begin();
					it != actions.end(); ++it )
	{
		if( ( *it )->isChecked() == TRUE )
		{
			selected_ports.push_back( ( *it )->text() );
		}
	}

	m_readablePorts->clear();
	const QStringList & rp = engine::getMixer()->getMIDIClient()->
								readablePorts();
	// now insert new ports and restore selections
	for( QStringList::const_iterator it = rp.begin(); it != rp.end(); ++it )
	{
		QAction * item = m_readablePorts->addAction( *it );
		item->setCheckable( TRUE );
		if( selected_ports.indexOf( *it ) != -1 )
		{
			item->setChecked( TRUE );
		}
	}
}




void midiTabWidget::writeablePortsChanged( void )
{
	// first save all selected ports
	QStringList selected_ports;
	QList<QAction *> actions = m_writeablePorts->actions();
	for( QList<QAction *>::iterator it = actions.begin();
					it != actions.end(); ++it )
	{
		if( ( *it )->isChecked() == TRUE )
		{
			selected_ports.push_back( ( *it )->text() );
		}
	}

	m_writeablePorts->clear();
	const QStringList & wp = engine::getMixer()->getMIDIClient()->
							writeablePorts();
	// now insert new ports and restore selections
	for( QStringList::const_iterator it = wp.begin(); it != wp.end(); ++it )
	{
		QAction * item = m_writeablePorts->addAction( *it );
		item->setCheckable( TRUE );
		if( selected_ports.indexOf( *it ) != -1 )
		{
			item->setChecked( TRUE );
		}
	}
}




void midiTabWidget::activatedReadablePort( QAction * _item )
{
	// make sure, MIDI-port is configured for input
	if( _item->isChecked() == TRUE &&
		m_midiPort->mode() != midiPort::INPUT &&
		m_midiPort->mode() != midiPort::DUPLEX )
	{
		m_receiveEnabledModel.setValue( TRUE );
	}
	engine::getMixer()->getMIDIClient()->subscribeReadablePort( m_midiPort,
				_item->text(), !_item->isChecked() );
}




void midiTabWidget::activatedWriteablePort( QAction * _item )
{
	// make sure, MIDI-port is configured for output
	if( _item->isChecked() == TRUE &&
		m_midiPort->mode() != midiPort::OUTPUT &&
		m_midiPort->mode() != midiPort::DUPLEX )
	{
		m_sendEnabledModel.setValue( TRUE );
	}
	engine::getMixer()->getMIDIClient()->subscribeWriteablePort( m_midiPort,
				_item->text(), !_item->isChecked() );
}



#include "midi_tab_widget.moc"


#endif
