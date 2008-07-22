#ifndef SINGLE_SOURCE_COMPILE

/*
 * controller_connection_dialog.cpp - dialog allowing the user to create and
 *	modify links between controllers and models
 *
 * Copyright (c) 2008  Paul Giblock <drfaygo/at/gmail.com>
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


#include <QtGui/QLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QScrollArea>

#include "controller_connection_dialog.h"
#include "controller_connection.h"
#include "lcd_spinbox.h"
#include "led_checkbox.h"
#include "combobox.h"
#include "tab_widget.h"
#include "group_box.h"
#include "midi_controller.h"
#include "midi_client.h"
#include "midi_port_menu.h"
#include "midi.h"
#include "song.h"
#include "tool_button.h"

#include "gui_templates.h"
#include "embed.h"

class autoDetectMidiController : public midiController
{
public:
	autoDetectMidiController( model * _parent ) :
		midiController( _parent ),
		m_detectedMidiChannel( 0 ),
		m_detectedMidiController( 0 )
	{
		updateName();
	}


	virtual ~autoDetectMidiController()
	{
	}


	virtual void processInEvent( const midiEvent & _me,
					const midiTime & _time )
	{
		if( _me.m_type == MidiControlChange &&
			( m_midiPort.inputChannel() == _me.m_channel + 1 ||
				m_midiPort.inputChannel() == 0 ) )
		{
			m_detectedMidiChannel = _me.m_channel + 1;
			m_detectedMidiController = ( _me.m_data.m_bytes[0] & 0x7F ) + 1;

			emit valueChanged();
		}
	}


	int m_detectedMidiChannel;
	int m_detectedMidiController;

	// Would be a nice copy ctor, but too hard to add copy ctor because
	// model has none.
	midiController * copyToMidiController( model * _parent )
	{
		midiController * c = new midiController( _parent );
		c->m_midiPort.setInputChannel( m_midiPort.inputChannel() );
		c->m_midiPort.setInputController( m_midiPort.inputController() );
		c->subscribeReadablePorts( m_midiPort.readablePorts() );
		c->updateName();

		return c;
	}


	void useDetected( void )
	{
		m_midiPort.setInputChannel( m_detectedMidiChannel );
		m_midiPort.setInputController( m_detectedMidiController );
	}


public slots:
	void reset( void )
	{
		m_midiPort.setInputChannel( 0 );
		m_midiPort.setInputController( 0 );
	}

};




controllerConnectionDialog::controllerConnectionDialog( QWidget * _parent, 
		const automatableModel * _target_model
	) :
	QDialog( _parent ),
	m_readablePorts( NULL ),
	m_midiAutoDetect( FALSE ),
	m_controller( NULL ),
	m_targetModel( _target_model ),
	m_midiController( NULL )
{
	setWindowIcon( embed::getIconPixmap( "setup_audio" ) );
	setWindowTitle( tr( "Connection Settings" ) );
	setModal( TRUE );

	// Midi stuff
	m_midiGroupBox = new groupBox( tr( "MIDI CONTROLLER" ), this );
	m_midiGroupBox->setGeometry( 8, 10, 240, 80 );
	connect( m_midiGroupBox->model(), SIGNAL( dataChanged() ),
			this, SLOT( midiToggled() ) );
	
	m_midiChannelSpinBox = new lcdSpinBox( 2, m_midiGroupBox,
			tr( "Input channel" ) );
	m_midiChannelSpinBox->addTextForValue( 0, "--" );
	m_midiChannelSpinBox->setLabel( tr( "CHANNEL" ) );
	m_midiChannelSpinBox->move( 8, 24 );

	m_midiControllerSpinBox = new lcdSpinBox( 3, m_midiGroupBox,
			tr( "Input controller" ) );
	m_midiControllerSpinBox->addTextForValue( 0, "---" );
	m_midiControllerSpinBox->setLabel( tr( "CONTROLLER" ) );
	m_midiControllerSpinBox->move( 68, 24 );
	

	m_midiAutoDetectCheckBox =
			new ledCheckBox( tr("Auto Detect"),
				m_midiGroupBox, tr("Auto Detect") );
	m_midiAutoDetectCheckBox->setModel( &m_midiAutoDetect );
	m_midiAutoDetectCheckBox->move( 8, 60 );
	connect( &m_midiAutoDetect, SIGNAL( dataChanged() ),
			this, SLOT( autoDetectToggled() ) );

	// when using with non-raw-clients we can provide buttons showing
	// our port-menus when being clicked
	if( !engine::getMixer()->getMIDIClient()->isRaw() )
	{
		m_readablePorts = new midiPortMenu( midiPort::Input );

		toolButton * rp_btn = new toolButton( m_midiGroupBox );
		rp_btn->setText( tr( "MIDI-devices to receive "
						"MIDI-events from" ) );
		rp_btn->setIcon( embed::getIconPixmap( "midi_in" ) );
		rp_btn->setGeometry( 140, 24, 32, 32 );
		rp_btn->setMenu( m_readablePorts );
		rp_btn->setPopupMode( QToolButton::InstantPopup );
	}


	// User stuff
	m_userGroupBox = new groupBox( tr( "USER CONTROLLER" ), this );
	m_userGroupBox->setGeometry( 8, 100, 240, 60 );
	connect( m_userGroupBox->model(), SIGNAL( dataChanged() ),
			this, SLOT( userToggled() ) );

	m_userController = new comboBox( m_userGroupBox, "Controller" );
	m_userController->setGeometry( 10, 24, 200, 22 );

	for( int i = 0; i < engine::getSong()->controllers().size(); ++i )
	{
		controller * c = engine::getSong()->controllers().at( i );
		m_userController->model()->addItem( c->name() );
	}
	

	// Mapping functions
	m_mappingBox = new tabWidget( tr( "MAPPING FUNCTION" ), this );
	m_mappingBox->setGeometry( 8, 170, 240, 64 );
	m_mappingFunction = new QLineEdit( m_mappingBox );
	m_mappingFunction->setGeometry( 10, 20, 170, 16 );
	m_mappingFunction->setText( "input" );
	m_mappingFunction->setReadOnly( TRUE );


	// Buttons
	QWidget * buttons = new QWidget( this );
	buttons->setGeometry( 8, 240, 240, 32 );

	QHBoxLayout * btn_layout = new QHBoxLayout( buttons );
	btn_layout->setSpacing( 0 );
	btn_layout->setMargin( 0 );
	
	QPushButton * select_btn = new QPushButton( 
					embed::getIconPixmap( "add" ),
					tr( "OK" ), buttons );
	connect( select_btn, SIGNAL( clicked() ), 
				this, SLOT( selectController() ) );
	
	QPushButton * cancel_btn = new QPushButton( 
					embed::getIconPixmap( "cancel" ),
					tr( "Cancel" ), buttons );
	connect( cancel_btn, SIGNAL( clicked() ),
				this, SLOT( reject() ) );

	btn_layout->addStretch();
	btn_layout->addSpacing( 10 );
	btn_layout->addWidget( select_btn );
	btn_layout->addSpacing( 10 );
	btn_layout->addWidget( cancel_btn );
	btn_layout->addSpacing( 10 );

	setFixedSize( 256, 280 );

	// Crazy MIDI View stuff
	
	// TODO, handle by making this a model for the Dialog "view"
	controllerConnection * cc = NULL;
	if( m_targetModel )
	{
		cc = m_targetModel->getControllerConnection();

		if( cc && cc->getController()->type() != 
				controller::DummyController && engine::getSong() )
		{
			if ( cc->getController()->type() == 
					controller::MidiController )
			{
				m_midiGroupBox->model()->setValue( TRUE );
				// ensure controller is created
				midiToggled();
			
				midiController * cont = 
					(midiController*)( cc->getController() );
				m_midiChannelSpinBox->model()->setValue(
						cont->m_midiPort.inputChannel() );
				m_midiControllerSpinBox->model()->setValue(
						cont->m_midiPort.inputController() );

				m_midiController->subscribeReadablePorts( static_cast<midiController*>( cc->getController() )->m_midiPort.readablePorts() );
			}
			else
			{
				int idx = engine::getSong()->controllers().indexOf(
						cc->getController() );

				if( idx >= 0 )
				{
					m_userGroupBox->model()->setValue( TRUE );
					m_userController->model()->setValue( idx );
				}
			}
		}
	}

	if( !cc )
	{
		m_midiGroupBox->model()->setValue( TRUE );
	}

	show();
}




controllerConnectionDialog::~controllerConnectionDialog()
{
	delete m_readablePorts;

	if( m_midiController )
		delete m_midiController;
}




void controllerConnectionDialog::selectController( void )
{
	// Midi
	if( m_midiGroupBox->model()->value() > 0 )
	{
		if( m_midiControllerSpinBox->model()->value() > 0 )
		{
			midiController * mc;
			mc = m_midiController->copyToMidiController(
					engine::getSong() );
	
			/*
			if( m_targetModel->getTrack() && 
					!m_targetModel->getTrack()->displayName().isEmpty() )
			{
				mc->m_midiPort.setName( QString( "%1 (%2)" ).
						arg( m_targetModel->getTrack()->displayName() ).
						arg( m_targetModel->displayName() ) );
			}
			else
			{
				mc->m_midiPort.setName( m_targetModel->displayName() );
			}
			*/
			mc->m_midiPort.setName( m_targetModel->fullDisplayName() );
			m_controller = mc;
		}
	}
	// User
	else 
	{
		if( m_userGroupBox->model()->value() > 0 && 
				engine::getSong()->controllers().size() )
		{
			m_controller = engine::getSong()->controllers().at( 
					m_userController->model()->value() );
		}
	}
	accept();
}




void controllerConnectionDialog::midiToggled( void )
{
	int enabled = m_midiGroupBox->model()->value();
	if( enabled != 0 )
	{
		if( m_userGroupBox->model()->value() != 0 )
		{
			m_userGroupBox->model()->setValue( 0 );
		}

		if( !m_midiController )
		{
			m_midiController = new autoDetectMidiController( engine::getSong() );
			m_midiChannelSpinBox->setModel( 
					&m_midiController->m_midiPort.m_inputChannelModel );
			m_midiControllerSpinBox->setModel( 
					&m_midiController->m_midiPort.m_inputControllerModel );
			m_readablePorts->setModel( &m_midiController->m_midiPort );


			connect( m_midiController, SIGNAL( valueChanged() ), 
				this, SLOT( midiValueChanged() ) );
		}
	}
	else
	{
		m_midiAutoDetect.setValue( FALSE );
	}

	m_midiChannelSpinBox->setEnabled( enabled );
	m_midiControllerSpinBox->setEnabled( enabled );
	m_midiAutoDetectCheckBox->setEnabled( enabled );
}




void controllerConnectionDialog::userToggled( void )
{
	int enabled = m_userGroupBox->model()->value();
	if( enabled != 0 && m_midiGroupBox->model()->value() != 0 )
	{
		m_midiGroupBox->model()->setValue( 0 );
	}

	m_userController->setEnabled( enabled );
}




void controllerConnectionDialog::autoDetectToggled( void )
{
	if( m_midiAutoDetect.value() )
	{
		m_midiController->reset();
	}
}




void controllerConnectionDialog::midiValueChanged( void )
{
	if( m_midiAutoDetect.value() )
	{
		m_midiController->useDetected();

		m_midiAutoDetect.setValue( FALSE );
	}
}




#include "moc_controller_connection_dialog.cxx"

#endif
