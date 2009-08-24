/*
 * setup_dialog_mcl.cpp - dialog for setting up MIDI Control Listener
 *
 * Copyright (c) 2009 Achim Settelmeier <lmms/at/m1.sirlab.de>
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
#include <QtGui/QPushButton>
#include <QtGui/QComboBox>

#include "embed.h"
#include "group_box.h"
#include "lcd_spinbox.h"

#include "setup_dialog.h"
#include "setup_dialog_mcl.h"
#include "PianoView.h"


setupDialogMCL::setupDialogMCL( setupDialog * _parent ) :
	m_parent( _parent ),
	m_keysActive( true ),
	m_mep(),
	m_pianoModel( &m_mep )
{
	setWindowTitle( tr( "New action" ) );
	setModal( true );
	resize( 270, 330 );
	
	// ok/cancel buttons
	QWidget * buttons = new QWidget( this );
	QHBoxLayout * buttonLayout = new QHBoxLayout( buttons );
	buttonLayout->setSpacing( 0 );
	buttonLayout->setMargin( 0 );
	QPushButton * okButton = new QPushButton(
					embed::getIconPixmap( "apply" ),
						tr( "OK" ), buttons );
	connect( okButton, SIGNAL( clicked() ), this, SLOT( accept() ) );
	
	QPushButton * cancelButton = new QPushButton(
					embed::getIconPixmap( "cancel" ),
							tr( "Cancel" ),
							buttons );
	connect( cancelButton, SIGNAL( clicked() ), this, SLOT( reject() ) );
	
	buttonLayout->addStretch();
	buttonLayout->addSpacing( 10 );
	buttonLayout->addWidget( okButton );
	buttonLayout->addSpacing( 10 );
	buttonLayout->addWidget( cancelButton );
	buttonLayout->addSpacing( 10 );
	
	// container for settings, right above buttons
	QWidget * settings = new QWidget( this );
	settings->setGeometry( 10, 10, 280, 100 );
	
	// put window together
	QVBoxLayout * vlayout = new QVBoxLayout( this );
	vlayout->setSpacing( 0 );
	vlayout->setMargin( 0 );
	vlayout->addWidget( settings );
	vlayout->addSpacing( 10 );
	vlayout->addWidget( buttons );
	vlayout->addSpacing( 10 );
	vlayout->addStretch();
	
	// keyboard group
	m_actionKeyGroupBox = new groupBox( tr( "MIDI KEYBOARD" ),
						settings );
	m_actionKeyGroupBox->setFixedHeight( 160 );
	m_actionKeyGroupBox->ledButton()->setChecked( m_keysActive );
	connect( m_actionKeyGroupBox->ledButton(), SIGNAL( clicked() ), 
		this, SLOT( clickedKeyBox() ) );
	
	// controller group
	m_actionControllerGroupBox = new groupBox( tr( "MIDI CONTROLLER" ),
							settings );
	m_actionControllerGroupBox->setFixedHeight( 100 );
	m_actionControllerGroupBox->ledButton()->setChecked( ! m_keysActive );
	connect( m_actionControllerGroupBox->ledButton(), SIGNAL( clicked() ), 
		this, SLOT( clickedControllerBox() ) );
	
	// put settings box together
	QVBoxLayout * settingsLayout = new QVBoxLayout( settings );
	settingsLayout->setSpacing( 0 );
	settingsLayout->setMargin( 0 );
	settingsLayout->addWidget( m_actionControllerGroupBox );
	settingsLayout->addSpacing( 10 );
	settingsLayout->addWidget( m_actionKeyGroupBox );
	settingsLayout->addSpacing( 10 );
	settingsLayout->addStretch();
	
	// populate keys box
	m_actionsKeyBox = new QComboBox( m_actionKeyGroupBox );
	m_actionsKeyBox->setGeometry( 10, 20, 150, 22 );
	for( int i = 0; i < MidiControlListener::NumActions; ++i )
	{
		MidiControlListener::ActionNameMap action =
			MidiControlListener::action2ActionNameMap(
				(MidiControlListener::EventAction) i );
		if( !action.name.isEmpty() )
		{
			m_actionsKeyBox->addItem( action.name );
		}
	}
	connect( m_actionsKeyBox, SIGNAL( currentIndexChanged( int ) ),
		this, SLOT( clickedKeyBox() ) );
	
	QWidget * pianoWidget = new QWidget( m_actionKeyGroupBox );
	pianoWidget->move( 10, 60 );
	
	PianoView * pianoViewWidget = new PianoView( pianoWidget );
	pianoViewWidget->setFixedSize( 250, 84 );
	pianoViewWidget->setModel( &m_pianoModel );
	m_mep.baseNoteModel()->setValue( 60 );
	m_mep.setUpdateBaseNote( true );
	connect( pianoViewWidget, SIGNAL( keyPressed( int ) ),
		this, SLOT( clickedKeyBox() ) );
	connect( pianoViewWidget, SIGNAL( baseNoteChanged() ),
		this, SLOT( clickedKeyBox() ) );
	
	// populate controller box
	m_actionsControllerBox = new QComboBox( m_actionControllerGroupBox );
	m_actionsControllerBox->setGeometry( 10, 30, 150, 22 );
	for( int i = 0; i < MidiControlListener::NumActions; ++i )
	{
		MidiControlListener::ActionNameMap action =
			MidiControlListener::action2ActionNameMap(
				(MidiControlListener::EventAction) i );
		if( !action.name.isEmpty() && 
			action.action != MidiControlListener::ActionControl )
		{
			m_actionsControllerBox->addItem( action.name );
		}
	}
	connect( m_actionsControllerBox, SIGNAL( currentIndexChanged( int ) ),
		this, SLOT( clickedControllerBox() ) );
	
	m_controllerSbModel = new lcdSpinBoxModel( /* this */ );
	m_controllerSbModel->setRange( 0, 127 );
	m_controllerSbModel->setStep( 1 );
	m_controllerSbModel->setValue( 23 );
	lcdSpinBox * controllerSb = new lcdSpinBox( 3,
						m_actionControllerGroupBox );
	controllerSb->setModel( m_controllerSbModel );
	controllerSb->setLabel( tr( "CONTROLLER" ) );
	controllerSb->move( 20, 60 );
	connect( controllerSb, SIGNAL( manualChange() ),
		this, SLOT( clickedControllerBox() ) );
	
	// widgets setup done
	show();
}




setupDialogMCL::~setupDialogMCL()
{
}




void setupDialogMCL::accept()
{
	if( m_keysActive )
	{
		MidiControlListener::ActionNameMap action =
			MidiControlListener::actionName2ActionNameMap(
				m_actionsKeyBox->currentText() );
		
		m_parent->mclAddKeyAction( m_mep.baseNoteModel()->value(), 
					  action.action );
	}
	else
	{
		MidiControlListener::ActionNameMap action =
			MidiControlListener::actionName2ActionNameMap(
				m_actionsControllerBox->currentText() );
		
		m_parent->mclAddControllerAction( m_controllerSbModel->value(), 
						 action.action );
	}
	QDialog::accept();
}




void setupDialogMCL::clickedKeyBox()
{
	m_keysActive = true;
	m_actionKeyGroupBox->ledButton()->setChecked( true );
	m_actionControllerGroupBox->ledButton()->setChecked( false );
}




void setupDialogMCL::clickedControllerBox()
{
	m_keysActive = false;
	m_actionKeyGroupBox->ledButton()->setChecked( false );
	m_actionControllerGroupBox->ledButton()->setChecked( true );
}




#include "moc_setup_dialog_mcl.cxx"

