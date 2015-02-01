/*
 * InstrumentMidiIOView.cpp - MIDI-IO-View
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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
#include <QtGui/QLabel>
#include <QtGui/QLayout>

#include "InstrumentMidiIOView.h"
#include "MidiPortMenu.h"
#include "engine.h"
#include "embed.h"
#include "group_box.h"
#include "gui_templates.h"
#include "LcdSpinBox.h"
#include "MidiClient.h"
#include "Mixer.h"
#include "tooltip.h"



InstrumentMidiIOView::InstrumentMidiIOView( QWidget* parent ) :
	QWidget( parent ),
	ModelView( NULL, this ),
	m_rpBtn( NULL ),
	m_wpBtn( NULL )
{
	QVBoxLayout* layout = new QVBoxLayout( this );
	layout->setMargin( 5 );
	m_midiInputGroupBox = new groupBox( tr( "ENABLE MIDI INPUT" ) );
	layout->addWidget( m_midiInputGroupBox );

	QHBoxLayout* midiInputLayout = new QHBoxLayout( m_midiInputGroupBox );
	midiInputLayout->setContentsMargins( 8, 18, 8, 8 );
	midiInputLayout->setSpacing( 6 );

	m_inputChannelSpinBox = new LcdSpinBox( 2, m_midiInputGroupBox );
	m_inputChannelSpinBox->addTextForValue( 0, "--" );
	m_inputChannelSpinBox->setLabel( tr( "CHANNEL" ) );
	m_inputChannelSpinBox->setEnabled( false );
	midiInputLayout->addWidget( m_inputChannelSpinBox );

	m_fixedInputVelocitySpinBox = new LcdSpinBox( 3, m_midiInputGroupBox );
	m_fixedInputVelocitySpinBox->setDisplayOffset( 1 );
	m_fixedInputVelocitySpinBox->addTextForValue( 0, "---" );
	m_fixedInputVelocitySpinBox->setLabel( tr( "VELOCITY" ) );
	m_fixedInputVelocitySpinBox->setEnabled( false );
	midiInputLayout->addWidget( m_fixedInputVelocitySpinBox );
	midiInputLayout->addStretch();

	connect( m_midiInputGroupBox->ledButton(), SIGNAL( toggled( bool ) ),
			m_inputChannelSpinBox, SLOT( setEnabled( bool ) ) );
	connect( m_midiInputGroupBox->ledButton(), SIGNAL( toggled( bool ) ),
		m_fixedInputVelocitySpinBox, SLOT( setEnabled( bool ) ) );



	m_midiOutputGroupBox = new groupBox( tr( "ENABLE MIDI OUTPUT" ) );
	layout->addWidget( m_midiOutputGroupBox );

	QHBoxLayout* midiOutputLayout = new QHBoxLayout( m_midiOutputGroupBox );
	midiOutputLayout->setContentsMargins( 8, 18, 8, 8 );
	midiOutputLayout->setSpacing( 6 );

	m_outputChannelSpinBox = new LcdSpinBox( 2, m_midiOutputGroupBox );
	m_outputChannelSpinBox->setLabel( tr( "CHANNEL" ) );
	m_outputChannelSpinBox->setEnabled( false );
	midiOutputLayout->addWidget( m_outputChannelSpinBox );

	m_fixedOutputVelocitySpinBox = new LcdSpinBox( 3, m_midiOutputGroupBox );
	m_fixedOutputVelocitySpinBox->setDisplayOffset( 1 );
	m_fixedOutputVelocitySpinBox->addTextForValue( 0, "---" );
	m_fixedOutputVelocitySpinBox->setLabel( tr( "VELOCITY" ) );
	m_fixedOutputVelocitySpinBox->setEnabled( false );
	midiOutputLayout->addWidget( m_fixedOutputVelocitySpinBox );

	m_outputProgramSpinBox = new LcdSpinBox( 3, m_midiOutputGroupBox );
	m_outputProgramSpinBox->setLabel( tr( "PROGRAM" ) );
	m_outputProgramSpinBox->setEnabled( false );
	midiOutputLayout->addWidget( m_outputProgramSpinBox );

	m_fixedOutputNoteSpinBox = new LcdSpinBox( 3, m_midiOutputGroupBox );
	m_fixedOutputNoteSpinBox->setDisplayOffset( 1 );
	m_fixedOutputNoteSpinBox->addTextForValue( 0, "---" );
	m_fixedOutputNoteSpinBox->setLabel( tr( "NOTE" ) );
	m_fixedOutputNoteSpinBox->setEnabled( false );
	midiOutputLayout->addWidget( m_fixedOutputNoteSpinBox );
	midiOutputLayout->addStretch();

	connect( m_midiOutputGroupBox->ledButton(), SIGNAL( toggled( bool ) ),
			m_outputChannelSpinBox, SLOT( setEnabled( bool ) ) );
	connect( m_midiOutputGroupBox->ledButton(), SIGNAL( toggled( bool ) ),
		m_fixedOutputVelocitySpinBox, SLOT( setEnabled( bool ) ) );
	connect( m_midiOutputGroupBox->ledButton(), SIGNAL( toggled( bool ) ),
			m_outputProgramSpinBox, SLOT( setEnabled( bool ) ) );
	connect( m_midiOutputGroupBox->ledButton(), SIGNAL( toggled( bool ) ),
		m_fixedOutputNoteSpinBox, SLOT( setEnabled( bool ) ) );

	if( !engine::mixer()->midiClient()->isRaw() )
	{
		m_rpBtn = new QToolButton;
		m_rpBtn->setMinimumSize( 32, 32 );
		m_rpBtn->setText( tr( "MIDI devices to receive MIDI events from" ) );
		m_rpBtn->setIcon( embed::getIconPixmap( "piano" ) );
		m_rpBtn->setPopupMode( QToolButton::InstantPopup );
		midiInputLayout->insertSpacing( 0, 4 );
		midiInputLayout->insertWidget( 0, m_rpBtn );

		m_wpBtn = new QToolButton;
		m_wpBtn->setMinimumSize( 32, 32 );
		m_wpBtn->setText( tr( "MIDI devices to send MIDI events to" ) );
		m_wpBtn->setIcon( embed::getIconPixmap( "piano" ) );
		m_wpBtn->setPopupMode( QToolButton::InstantPopup );
		midiOutputLayout->insertSpacing( 0, 4 );
		midiOutputLayout->insertWidget( 0, m_wpBtn );
	}

#define PROVIDE_CUSTOM_BASE_VELOCITY_UI
#ifdef PROVIDE_CUSTOM_BASE_VELOCITY_UI
	groupBox* baseVelocityGroupBox = new groupBox( tr( "CUSTOM BASE VELOCITY" ) );
	layout->addWidget( baseVelocityGroupBox );

	QVBoxLayout* baseVelocityLayout = new QVBoxLayout( baseVelocityGroupBox );
	baseVelocityLayout->setContentsMargins( 8, 18, 8, 8 );
	baseVelocityLayout->setSpacing( 6 );

	QLabel* baseVelocityHelp = new QLabel( tr( "Specify the velocity normalization base for MIDI-based instruments at note volume 100%" ) );
	baseVelocityHelp->setWordWrap( true );
    baseVelocityHelp->setFont( pointSize<8>( baseVelocityHelp->font() ) );

	baseVelocityLayout->addWidget( baseVelocityHelp );

	m_baseVelocitySpinBox = new LcdSpinBox( 3, baseVelocityGroupBox );
	m_baseVelocitySpinBox->setLabel( tr( "BASE VELOCITY" ) );
	m_baseVelocitySpinBox->setEnabled( false );
	baseVelocityLayout->addWidget( m_baseVelocitySpinBox );

	connect( baseVelocityGroupBox->ledButton(), SIGNAL( toggled( bool ) ),
			m_baseVelocitySpinBox, SLOT( setEnabled( bool ) ) );
#endif

	layout->addStretch();
}




InstrumentMidiIOView::~InstrumentMidiIOView()
{
}




void InstrumentMidiIOView::modelChanged()
{
	MidiPort * mp = castModel<MidiPort>();

	m_midiInputGroupBox->setModel( &mp->m_readableModel );
	m_inputChannelSpinBox->setModel( &mp->m_inputChannelModel );
	m_fixedInputVelocitySpinBox->setModel( &mp->m_fixedInputVelocityModel );

	m_midiOutputGroupBox->setModel( &mp->m_writableModel );
	m_outputChannelSpinBox->setModel( &mp->m_outputChannelModel );
	m_fixedOutputVelocitySpinBox->setModel( &mp->m_fixedOutputVelocityModel );
	m_fixedOutputNoteSpinBox->setModel( &mp->m_fixedOutputNoteModel );
	m_outputProgramSpinBox->setModel( &mp->m_outputProgramModel );

#ifdef PROVIDE_CUSTOM_BASE_VELOCITY_UI
	m_baseVelocitySpinBox->setModel( &mp->m_baseVelocityModel );
#endif

	if( m_rpBtn )
	{
		m_rpBtn->setMenu( mp->m_readablePortsMenu );
	}
	if( m_wpBtn )
	{
		m_wpBtn->setMenu( mp->m_writablePortsMenu );
	}
}

#include "moc_InstrumentMidiIOView.cxx"
