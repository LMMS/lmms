/*
 * ControllerConnectionDialog.cpp - dialog allowing the user to create and
 *	modify links between controllers and models
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail.com>
 *
 * This file is part of LMMS - https://lmms.io
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

#include <QLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QMessageBox>
#include <QLabel>

#include "ControllerConnectionDialog.h"
#include "ControllerConnection.h"
#include "MidiCustomController.h"
#include "MidiClient.h"
#include "MidiPortMenu.h"
#include "Mixer.h"
#include "LcdSpinBox.h"
#include "LedCheckbox.h"
#include "ComboBox.h"
#include "GroupBox.h"
#include "Song.h"
#include "ToolButton.h"

#include "gui_templates.h"
#include "embed.h"


QString enumToString(MidiEventTypes midi)
{
	switch (midi){
		case 	MidiNoteOff:
			return QString("MidiNoteOff");
			break;
		case MidiNoteOn:
			return QString("MidiNoteOn");
			break;
		case MidiKeyPressure:
			return QString("MidiKeyPressure");
			break;
		case MidiControlChange:
			return QString("MidiControlChange");
			break;
		case MidiProgramChange:
			return QString("MidiProgramChange");
			break;
		case MidiChannelPressure:
			return QString("MidiChannelPressure");
			break;
		case MidiPitchBend:
			return QString("MidiPitchBend");
			break;
		default:
			return QString("Unheld message");
			break;
		}
}



class AutoDetectMidiController : public MidiCustomController
{
public:
	AutoDetectMidiController( Model* parent ) :
		MidiCustomController( parent ),
		m_detectedMidiChannel( 0 ),
		m_detectedMidiController( 0 )
	{
		updateName();
	}


	virtual ~AutoDetectMidiController()
	{
	}


	virtual void processInEvent( const MidiEvent& event, const MidiTime& time, f_cnt_t offset = 0 )
	{
	//ORIG
//		if( event.type() == MidiControlChange &&
//			( m_midiPort.inputChannel() == 0 || m_midiPort.inputChannel() == event.channel() + 1 ) )
 //ORIG

		//RIKIS	- detecting all midi inputs
		if(	( m_midiPort.inputChannel() == 0 || m_midiPort.inputChannel() == event.channel() + 1 ) )
			{
				m_detectedMidiChannel = event.channel() + 1;
				m_detectedMidiController = event.controllerNumber() + 1;
				m_detectedMidiPort = Engine::mixer()->midiClient()->sourcePortName( event );

				useDetected(); //setta il controller
				MidiCustomController::processInEvent(event,time,offset); //Colling base class event to trigger actions and signal
//				emit valueChanged(); //Not necessary anymore
			}
	}


	// Would be a nice copy ctor, but too hard to add copy ctor because
	// model has none.
	MidiCustomController* copyToMidiController( Model* parent )
	{
		MidiCustomController* c = new MidiCustomController( parent );
		c->m_midiPort.setInputChannel( m_midiPort.inputChannel() );
		c->m_midiPort.setInputController( m_midiPort.inputController() );
		c->subscribeReadablePorts( m_midiPort.readablePorts() );
		c->updateName();

		return c;
	}


	void useDetected()
	{
		m_midiPort.setInputChannel( m_detectedMidiChannel );
		m_midiPort.setInputController( m_detectedMidiController );

		const MidiPort::Map& map = m_midiPort.readablePorts();
		for( MidiPort::Map::ConstIterator it = map.begin(); it != map.end(); ++it )
		{
			m_midiPort.subscribeReadablePort( it.key(),
									m_detectedMidiPort.isEmpty() || ( it.key() == m_detectedMidiPort ) );
		}
	}


public slots:
	void reset()
	{
		m_midiPort.setInputChannel( 0 );
		m_midiPort.setInputController( 0 );
	}


private:
	int m_detectedMidiChannel;
	int m_detectedMidiController;
	QString m_detectedMidiPort;

} ;




ControllerConnectionDialog::ControllerConnectionDialog( QWidget * _parent, 
																												const AutomatableModel * _target_model ) :
	QDialog( _parent ),
	m_readablePorts( NULL ),
	m_midiAutoDetect( false ),
	m_controller( NULL ),
	m_targetModel( _target_model ),
	m_midiController( NULL )
{
	setWindowIcon( embed::getIconPixmap( "setup_audio" ) );
	setWindowTitle( tr( "Connection Settings" ) );
	setModal( true );


	// Midi stuff
	m_midiGroupBox = new GroupBox( tr( "MIDI CONTROLLER" ), this );
	//ORIGINAL
//m_midiGroupBox->setGeometry( 8, 10, 240, 80 );
	//RIKIS
	m_midiGroupBox->setGeometry( 8, 10, 330, 80 );
	//RIKIS
connect( m_midiGroupBox->model(), SIGNAL( dataChanged() ),
			this, SLOT( midiToggled() ) );

	m_midiChannelSpinBox = new LcdSpinBox( 2, m_midiGroupBox,
			tr( "Input channel" ) );
	m_midiChannelSpinBox->addTextForValue( 0, "--" );
	m_midiChannelSpinBox->setLabel( tr( "CHANNEL" ) );
	m_midiChannelSpinBox->move( 8, 24 );

	m_midiControllerSpinBox = new LcdSpinBox( 3, m_midiGroupBox,
			tr( "Input controller" ) );
	m_midiControllerSpinBox->addTextForValue( 0, "---" );
	m_midiControllerSpinBox->setLabel( tr( "CONTROLLER" ) );
	m_midiControllerSpinBox->move( 68, 24 );


	m_midiAutoDetectCheckBox =
			new LedCheckBox( tr("Auto Detect"),
				m_midiGroupBox, tr("Auto Detect") );
	m_midiAutoDetectCheckBox->setModel( &m_midiAutoDetect );
	m_midiAutoDetectCheckBox->move( 8, 60 );
	connect( &m_midiAutoDetect, SIGNAL( dataChanged() ),
			this, SLOT( autoDetectToggled() ) );

	// when using with non-raw-clients we can provide buttons showing
	// our port-menus when being clicked
	if( !Engine::mixer()->midiClient()->isRaw() )
	{
		m_readablePorts = new MidiPortMenu( MidiPort::Input );
		connect( m_readablePorts, SIGNAL( triggered( QAction * ) ),
				this, SLOT( enableAutoDetect( QAction * ) ) );
		ToolButton * rp_btn = new ToolButton( m_midiGroupBox );
		rp_btn->setText( tr( "MIDI-devices to receive "
						"MIDI-events from" ) );
		rp_btn->setIcon( embed::getIconPixmap( "piano" ) );
		rp_btn->setGeometry( 290, 24, 32, 32 );
		rp_btn->setMenu( m_readablePorts );
		rp_btn->setPopupMode( QToolButton::InstantPopup );

		//RIKIS
		QFont font;
		font.setPointSize(8);
		font.setBold(true);
		m_ControllerTypeLabel = new QLabel( m_midiGroupBox );
		m_ControllerTypeLabel->setStyleSheet("border: 1px solid grey ;color: red");
		m_ControllerTypeLabel->setFont(font);
		m_ControllerTypeLabel->setGeometry(150,24,105,15);
		m_ControllerTypeLabel->setMargin(1);
		m_ControllerTypeLabel->setText(tr("Controller message"));

		m_KeyLabel = new QLabel( m_midiGroupBox );
		m_KeyLabel->setGeometry(150,40,40,15);
		m_KeyLabel->setStyleSheet("border: 1px solid grey; color: orange");
		m_KeyLabel->setFont(font);
		m_KeyLabel->setMargin(1);
		m_KeyLabel->setText(tr("Key"));

		m_VelocityLabel = new QLabel( m_midiGroupBox );
		m_VelocityLabel->setGeometry(150,56,40,15);
		m_VelocityLabel->setStyleSheet("border: 1px solid grey; color: orange");
		m_VelocityLabel->setFont(font);
		m_VelocityLabel->setMargin(1);
		m_VelocityLabel->setText(tr("Velocity"));

		m_PitchLabel = new QLabel( m_midiGroupBox );
		m_PitchLabel->setGeometry(195,40,60,15);
		m_PitchLabel->setStyleSheet("border: 1px solid grey; color: orange");
		m_PitchLabel->setFont(font);
		m_PitchLabel->setMargin(1);
		m_PitchLabel->setText(tr("Pitch"));

		m_ValueLabel = new QLabel( m_midiGroupBox );
		m_ValueLabel->setGeometry(195,56,60,15);
		m_ValueLabel->setStyleSheet("border: 2px solid grey; color: green");
		m_ValueLabel->setFont(font);
		m_ValueLabel->setMargin(1);
		m_ValueLabel->setText(tr("Value"));

		//RIKIS
	}


	// User stuff
	m_userGroupBox = new GroupBox( tr( "USER CONTROLLER" ), this );
	//ORIGINAL
	//m_userGroupBox->setGeometry( 8, 100, 240, 60 );
	//RIKIS
	m_userGroupBox->setGeometry( 8, 100, 320, 60 );
	//RIKIS
	connect( m_userGroupBox->model(), SIGNAL( dataChanged() ),
			this, SLOT( userToggled() ) );

	m_userController = new ComboBox( m_userGroupBox, "Controller" );
	m_userController->setGeometry( 10, 24, 310, 22 );

	for( int i = 0; i < Engine::getSong()->controllers().size(); ++i )
	{
		Controller * c = Engine::getSong()->controllers().at( i );
		m_userController->model()->addItem( c->name() );
	}
	

	// Mapping functions
	m_mappingBox = new TabWidget( tr( "MAPPING FUNCTION" ), this );
	//ORIGINAL
	//m_mappingBox->setGeometry( 8, 170, 240, 64 );
	//RIKIS
	m_mappingBox->setGeometry( 8, 170, 330, 64 );
	//RIKIS

	m_mappingFunction = new QLineEdit( m_mappingBox );
	m_mappingFunction->setGeometry( 10, 20, 170, 16 );
	m_mappingFunction->setText( "input" );
	m_mappingFunction->setReadOnly( true );


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

//ORIGINAL
//	setFixedSize( 256, 280 );
//RIKIS
	setFixedSize( 350, 280 );

	// Crazy MIDI View stuff
	
	// TODO, handle by making this a model for the Dialog "view"
	ControllerConnection * cc = NULL;
	if( m_targetModel )
		{
			cc = m_targetModel->controllerConnection();

			if( cc && cc->getController()->type() != Controller::DummyController && Engine::getSong() )
				{
					if ( cc->getController()->type() == Controller::MidiController )
						{
							m_midiGroupBox->model()->setValue( true );
							// ensure controller is created
							midiToggled();

							MidiCustomController * cont = (MidiCustomController*)( cc->getController() );
							m_midiChannelSpinBox->model()->setValue( cont->m_midiPort.inputChannel() );
							m_midiControllerSpinBox->model()->setValue( cont->m_midiPort.inputController() );

							m_midiController->subscribeReadablePorts( static_cast<MidiController*>( cc->getController() )->m_midiPort.readablePorts() );
						}
					else
						{
							int idx = Engine::getSong()->controllers().indexOf( cc->getController() );

							if( idx >= 0 )
								{
									m_userGroupBox->model()->setValue( true );
									m_userController->model()->setValue( idx );
								}
						}
				}
		}

	if( !cc )
		{
			m_midiGroupBox->model()->setValue( true );
		}

	show();
}




ControllerConnectionDialog::~ControllerConnectionDialog()
{
	delete m_readablePorts;
//	delete m_rReadablePorts;

	delete m_midiController;
}




void ControllerConnectionDialog::selectController()
{
	// Midi
	if( m_midiGroupBox->model()->value() > 0 )
		{
			if( m_midiControllerSpinBox->model()->value() > 0 )
				{
					MidiCustomController * mc;
					mc = m_midiController->copyToMidiController( Engine::getSong() );
					mc->m_midiPort.setName( m_targetModel->fullDisplayName() );
					m_controller = mc;
				}
		}

	// User
	if( m_userGroupBox->model()->value() > 0 &&
			Engine::getSong()->controllers().size() )
		{
			m_controller = Engine::getSong()->controllers().at(
											 m_userController->model()->value() );
		}


	if( m_controller && m_controller->hasModel( m_targetModel ) )
		{
			QMessageBox::warning(this, tr("LMMS"), tr("Cycle Detected."));
			return;
		}

	accept();
}




void ControllerConnectionDialog::midiToggled()
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
			m_midiController = new AutoDetectMidiController( Engine::getSong() );

			MidiPort::Map map = m_midiController->m_midiPort.readablePorts();
			for( MidiPort::Map::Iterator it = map.begin(); it != map.end(); ++it )
			{
				it.value() = true;
			}
			m_midiController->subscribeReadablePorts( map );

			m_midiChannelSpinBox->setModel( &m_midiController->m_midiPort.m_inputChannelModel );
			m_midiControllerSpinBox->setModel( &m_midiController->m_midiPort.m_inputControllerModel );

			if( m_readablePorts )
			{
				m_readablePorts->setModel( &m_midiController->m_midiPort );
			}

			connect( m_midiController, SIGNAL( valueChanged() ), this, SLOT( midiValueChanged() ) );
		}
	}
	m_midiAutoDetect.setValue( enabled );

	m_midiChannelSpinBox->setEnabled( enabled );
	m_midiControllerSpinBox->setEnabled( enabled );
	m_midiAutoDetectCheckBox->setEnabled( enabled );
}


void ControllerConnectionDialog::userToggled()
{
	int enabled = m_userGroupBox->model()->value();
//	m_readablePorts->setEnabled(false);
	if( enabled != 0 && m_midiGroupBox->model()->value() != 0 )
	{
		m_midiGroupBox->model()->setValue( 0 );
	}

	m_userController->setEnabled( enabled );
}




void ControllerConnectionDialog::autoDetectToggled()
{
	if( m_midiAutoDetect.value() )
		{

			m_midiController->reset();
		}
}




void ControllerConnectionDialog::midiValueChanged()
{
	if( m_midiAutoDetect.value() )
	{
		m_midiController->useDetected();
		if( m_readablePorts )
		{
			m_readablePorts->updateMenu();
		}
	}
	//RIKIS
	setLabelText();
	//RIKIS
}


void ControllerConnectionDialog::enableAutoDetect( QAction * _a )
{
	if( _a->isChecked() )
	{
		m_midiAutoDetectCheckBox->model()->setValue( true );
		}
}

void ControllerConnectionDialog::setLabelText()
{
	m_ControllerTypeLabel->setText(enumToString(m_midiController->getMidiType()));
	m_KeyLabel->setText(QString::number(m_midiController->getMidiKey()));
	m_VelocityLabel->setText(QString::number(m_midiController->getMidiVelocity()));
	m_PitchLabel->setText(QString::number(m_midiController->getMidiPitchbend()));
	m_ValueLabel->setText(QString::number(m_midiController->getLastValue()));
}
