#ifndef SINGLE_SOURCE_COMPILE

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
#include <QComboBox>

#include "embed.h"
#include "group_box.h"
#include "lcd_spinbox.h"

#include "setup_dialog.h"
#include "setup_dialog_mcl.h"


setupDialogMCL::setupDialogMCL( setupDialog * _parent ) :
	m_parent( _parent ),
	m_keysActive( true )
{
	
	setWindowTitle( tr( "New action" ) );
	setModal( true );
	
	QWidget * buttons = new QWidget( this );
	QHBoxLayout * btn_layout = new QHBoxLayout( buttons );
	btn_layout->setSpacing( 0 );
	btn_layout->setMargin( 0 );
	QPushButton * ok_btn = new QPushButton( embed::getIconPixmap( "apply" ),
						tr( "OK" ), buttons );
	connect( ok_btn, SIGNAL( clicked() ), this, SLOT( accept() ) );

	QPushButton * cancel_btn = new QPushButton( embed::getIconPixmap(
								"cancel" ),
							tr( "Cancel" ),
							buttons );
	connect( cancel_btn, SIGNAL( clicked() ), this, SLOT( reject() ) );
	
	btn_layout->addStretch();
	btn_layout->addSpacing( 10 );
	btn_layout->addWidget( ok_btn );
	btn_layout->addSpacing( 10 );
	btn_layout->addWidget( cancel_btn );
	btn_layout->addSpacing( 10 );
	
	QWidget * settings = new QWidget( this );
	settings->setGeometry( 10, 10, 180, 100 );
	
	QVBoxLayout * vlayout = new QVBoxLayout( this );
	vlayout->setSpacing( 0 );
	vlayout->setMargin( 0 );
	vlayout->addWidget( settings );
	vlayout->addSpacing( 10 );
	vlayout->addWidget( buttons );
	vlayout->addSpacing( 10 );
	vlayout->addStretch();
	
	
	m_actionKey_gb = new groupBox( tr( "MIDI KEYBOARD" ),
						settings );
	m_actionKey_gb->setFixedHeight( 60 );
	m_actionKey_gb->ledButton()->setChecked( m_keysActive );
	connect( m_actionKey_gb->ledButton(), SIGNAL( clicked() ), 
		this, SLOT( clickedKeyBox(  ) ) );
	
	m_actionController_gb = new groupBox( tr( "MIDI CONTROLLER" ),
						settings );
	m_actionController_gb->setFixedHeight( 100 );
	m_actionController_gb->ledButton()->setChecked( ! m_keysActive );
	connect( m_actionController_gb->ledButton(), SIGNAL( clicked() ), 
		this, SLOT( clickedControllerBox( ) ) );
	
	
	QVBoxLayout * settings_vl = new QVBoxLayout( settings );
	settings_vl->setSpacing( 0 );
	settings_vl->setMargin( 0 );
	settings_vl->addWidget( m_actionController_gb );
	settings_vl->addSpacing( 10 );
	settings_vl->addWidget( m_actionKey_gb );
	settings_vl->addSpacing( 10 );
	settings_vl->addStretch();
	
	m_actionsKey_box = new QComboBox( m_actionKey_gb );
	m_actionsKey_box->setGeometry( 10, 20, 150, 22 );
	for( int i = 0; i < MidiControlListener::numActions; ++i )
	{
		MidiControlListener::ActionNameMap action = MidiControlListener::action2ActionNameMap( (MidiControlListener::EventAction) i );
		if( action.name != "" )
		{
			m_actionsKey_box->addItem( action.name );
		}
	}
	
	m_actionsController_box = new QComboBox( m_actionController_gb );
	m_actionsController_box->setGeometry( 10, 30, 150, 22 );
	for( int i = 0; i < MidiControlListener::numActions; ++i )
	{
		MidiControlListener::ActionNameMap action = MidiControlListener::action2ActionNameMap( (MidiControlListener::EventAction) i );
		if( action.name != "" && 
		   action.action != MidiControlListener::ActionControl )
		{
			m_actionsController_box->addItem( action.name );
		}
	}
	
	m_controllerSbModel = new lcdSpinBoxModel( /* this */ );
	m_controllerSbModel->setRange( 0, 127 );
	m_controllerSbModel->setStep( 1 );
	m_controllerSbModel->setValue( 63 );
	lcdSpinBox * controllerSb = new lcdSpinBox( 3, m_actionController_gb );
	controllerSb->setModel( m_controllerSbModel );
	controllerSb->setLabel( tr( "CONTROLLER" ) );
	controllerSb->move( 20, 60 );
	
	show();
}




setupDialogMCL::~setupDialogMCL( void )
{
}




void setupDialogMCL::accept( void )
{
	if( m_keysActive )
	{
		MidiControlListener::ActionNameMap action =
			MidiControlListener::actionName2ActionNameMap(
				m_actionsKey_box->currentText() );
		
		m_parent->mclAddKeyAction( 40, action.action );
	}
	else
	{
		MidiControlListener::ActionNameMap action =
			MidiControlListener::actionName2ActionNameMap(
				m_actionsController_box->currentText() );
		
		m_parent->mclAddControllerAction( m_controllerSbModel->value(), 
						 action.action );
	}
	QDialog::accept();
}




void setupDialogMCL::clickedKeyBox( void )
{
	m_keysActive = true;
	m_actionKey_gb->ledButton()->setChecked( true );
	m_actionController_gb->ledButton()->setChecked( false );
}




void setupDialogMCL::clickedControllerBox( void )
{
	m_keysActive = false;
	m_actionKey_gb->ledButton()->setChecked( false );
	m_actionController_gb->ledButton()->setChecked( true );
}




#include "moc_setup_dialog_mcl.cxx"


#endif
