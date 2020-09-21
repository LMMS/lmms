/*
 * MidiCCRackView.cpp - implementation of the MIDI CC rack widget
 *
 * Copyright (c) 2020 Ian Caio <iancaio_dev/at/hotmail.com>
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


#include <QWidget>
#include <QMdiSubWindow>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QScrollArea>

#include "MidiCCRackView.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "embed.h"
#include "ComboBox.h"
#include "ComboBoxModel.h"
#include "GroupBox.h"
#include "Knob.h"
#include "Track.h"
#include "InstrumentTrack.h"
#include "TrackContainer.h"
#include "BBTrackContainer.h"
#include "Engine.h"
#include "Song.h"
#include "SongEditor.h"
#include "BBEditor.h"


MidiCCRackView::MidiCCRackView( InstrumentTrack * track ) :
	QWidget(),
	m_track( track )
{
	setWindowIcon( embed::getIconPixmap( "midi_cc_rack" ) );
	setWindowTitle( tr("MIDI CC Rack") );

	QMdiSubWindow * subWin = gui->mainWindow()->addWindowedWidget( this );

	// Remove maximize button
	Qt::WindowFlags flags = subWin->windowFlags();
	flags &= ~Qt::WindowMaximizeButtonHint;
	subWin->setWindowFlags( flags );

	// Adjust window attributes, sizing and position
	subWin->setAttribute( Qt::WA_DeleteOnClose, false );
	subWin->move( 780, 50 );
	subWin->resize( 350, 300 );
	subWin->setFixedWidth( 350 );
	subWin->setMinimumHeight( 300 );
	subWin->hide();

	// Main window layout
	QVBoxLayout *mainLayout = new QVBoxLayout( this );

	// Track Selector Layout - Here we select which track we are sending CC messages to
	// We use a widget to be able to make this layout have a fixed height
	QWidget *trackToolBar = new QWidget();
	QHBoxLayout *trackToolBarLayout = new QHBoxLayout( trackToolBar );
	m_trackLabel = new QLabel( QString( tr( "Track: %1" ) ).arg( m_track->name() ) );

	trackToolBarLayout->addWidget(m_trackLabel);
	trackToolBar->setFixedHeight(40);

	// Knobs GroupBox - Here we have the MIDI CC controller knobs for the selected track
	m_midiCCGroupBox = new GroupBox( tr("MIDI CC Knobs:") );

	// Layout to keep scrollable area under the GroupBox header
	QVBoxLayout *knobsGroupBoxLayout = new QVBoxLayout();
	knobsGroupBoxLayout->setContentsMargins( 5, 16, 5, 5 );

	m_midiCCGroupBox->setLayout(knobsGroupBoxLayout);

	// Scrollable area + widget + its layout that will have all the knobs
	QScrollArea *knobsScrollArea = new QScrollArea();
	QWidget *knobsArea = new QWidget();
	QGridLayout *knobsAreaLayout = new QGridLayout();

	knobsArea->setLayout( knobsAreaLayout );
	knobsScrollArea->setWidget( knobsArea );
	knobsScrollArea->setWidgetResizable( true );

	knobsGroupBoxLayout->addWidget(knobsScrollArea);

	// Adds the controller knobs
	for(int i = 0; i < MidiControllerCount; i++){
		m_controllerKnob[i] = new Knob( knobBright_26 );
		m_controllerKnob[i]->setLabel( QString("CC %1").arg(QString::number(i)) );
		knobsAreaLayout->addWidget( m_controllerKnob[i], i/3, i%3 );
	}

	// Set all the models
	// Set the LED button to enable/disable the track midi cc
	m_midiCCGroupBox->setModel( m_track->m_midiCCEnable );

	// Set the model for each Knob
	for( int i = 0; i < MidiControllerCount; ++i ){
		m_controllerKnob[i]->setModel( m_track->m_midiCCModel[i] );
	}

	// Connection made to make sure the rack is destroyed if the track is destroyed
	connect( m_track, SIGNAL( destroyedTrack() ),
		this, SLOT( destroyRack() ) );

	// Connection to update the name of the track on the label
	connect( m_track, SIGNAL( nameChanged() ),
		this, SLOT( renameLabel() ) );

	// Adding everything to the main layout
	mainLayout->addWidget(trackToolBar);
	mainLayout->addWidget(m_midiCCGroupBox);
}

MidiCCRackView::~MidiCCRackView()
{
}

void MidiCCRackView::destroyRack()
{
	unsetModels();
	parentWidget()->close();
}

void MidiCCRackView::renameLabel()
{
	m_trackLabel->setText( QString( tr( "Track: %1" ) ).arg( m_track->name() ) );
}

void MidiCCRackView::unsetModels()
{
	m_midiCCGroupBox->unsetModel();

	for( int i = 0; i < MidiControllerCount; ++i )
	{
		m_controllerKnob[i]->unsetModel();
	}
}

void MidiCCRackView::saveSettings( QDomDocument & _doc,
				QDomElement & _this )
{
}

void MidiCCRackView::loadSettings( const QDomElement & _this )
{
}
