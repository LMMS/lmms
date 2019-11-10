/*
 * MasterToolBar.cpp - Widget within the main toolbar containing general song settings.
 *
 * Copyright (c) 2019 Lathigos <lathigos/at/tutanota.com>
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "MasterToolBar.h"

#include <QCloseEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include "AutomatableSlider.h"
#include "CPULoadWidget.h"
#include "embed.h"
#include "GuiApplication.h"
#include "IntegerSpinBox.h"
#include "MainWindow.h"
#include "MeterDialog.h"
#include "Song.h"
#include "TextFloat.h"
#include "ToolTip.h"
#include "SimpleVisualizationWidget.h"
#include "TimeDisplayWidget.h"


MasterToolBar::MasterToolBar( Song * song ) :
	m_song( song )
{
	// Get a pointer to the main toolbar:
	QWidget * tb = gui->mainWindow()->toolBar();
	
	// Create the custom widget to show:
	QHBoxLayout * frameLayout = new QHBoxLayout( this );
	frameLayout->setObjectName( "masterToolbarFrameLayout" );
	frameLayout->setSpacing( 24 );
	frameLayout->setContentsMargins( 18, 7, 18, 8 );
	
	frameLayout->addWidget( new TimeDisplayWidget( this ) );
	frameLayout->addSpacing( 2 );
	
	// Setup the tempo widget:
	QWidget * tempoWidget = new QWidget( this );
	tempoWidget->setMaximumHeight( 35 );
	QVBoxLayout * tempoWidgetLayout = new QVBoxLayout( tempoWidget );
	tempoWidgetLayout->setAlignment ( Qt::AlignRight );
	tempoWidgetLayout->setSpacing( 0 );
	tempoWidgetLayout->setContentsMargins( 0, 0, 0, 0 );
	
	m_tempoSpinBox = new IntegerSpinBox( 3, tempoWidget, tr( "Tempo" ) );
	m_tempoSpinBox->setModel( &m_song->m_tempoModel );
	ToolTip::add( m_tempoSpinBox, tr( "Tempo in BPM" ) );
	
	// Add caption label:
	QLabel * tempoLabel = new QLabel( tempoWidget );
	tempoLabel->setText( tr( "TEMPO" ) );
	tempoLabel->setObjectName( "integerDisplayTitle" );
	tempoWidgetLayout->addWidget( tempoLabel );
	tempoWidgetLayout->setAlignment( tempoLabel, Qt::AlignHCenter );
	
	tempoWidgetLayout->addWidget( m_tempoSpinBox );
	tempoWidgetLayout->setAlignment( m_tempoSpinBox, Qt::AlignHCenter );

	frameLayout->addWidget( tempoWidget );
	
	// Setup the time signature display widget:
	m_timeSigDisplay = new MeterDialog( this );
	m_timeSigDisplay->setModel( &m_song->m_timeSigModel );
	
	frameLayout->addWidget( m_timeSigDisplay );
	
	QWidget * masterControls = new QWidget( tb );
	QGridLayout * masterControlsLayout = new QGridLayout( masterControls );
	masterControlsLayout->setSpacing( 0 );
	masterControlsLayout->setContentsMargins( 0, 0, 0, 0 );
	
	// Setup the master volume slider:
	QLabel * masterVolumeLabel = new QLabel( tb );
	masterVolumeLabel->setText( tr("VOL") );
	masterVolumeLabel->setObjectName( "integerDisplayTitle" );
	masterControlsLayout->addWidget( masterVolumeLabel, 0, 0, Qt::AlignTop );
	
	m_masterVolumeSpinBox = new IntegerSpinBox( 3,
					"smallMasterSpinBox",
					masterControls,
					tr( "Master volume" ) );
	m_masterVolumeSpinBox->setModel( &m_song->m_masterVolumeModel );
	ToolTip::add( m_masterVolumeSpinBox, tr( "Master volume" ) );
	masterControlsLayout->addWidget( m_masterVolumeSpinBox, 0, 1,
					Qt::AlignTop | Qt::AlignRight );
	
	QLabel * masterVolumePercentLabel = new QLabel( masterControls );
	masterVolumePercentLabel->setText( tr("%") );
	masterVolumePercentLabel->setObjectName( "smallMasterSpinBox" );
	masterControlsLayout->addWidget( masterVolumePercentLabel, 0, 2,
					Qt::AlignTop | Qt::AlignRight );

	m_masterVolumeSpinBox->setZeroesVisible( false );
	
	m_masterVolumeSpinBox->setAlignment( Qt::AlignRight );
	
	// Setup the master pitch widget:
	QLabel * masterPitchLabel = new QLabel( tb );
	masterPitchLabel->setText( tr("PITCH") );
	masterPitchLabel->setObjectName( "integerDisplayTitle" );
	masterControlsLayout->addWidget( masterPitchLabel, 1, 0, Qt::AlignBottom );
	
	m_masterPitchSpinBox = new IntegerSpinBox( 3,
					"smallMasterSpinBox",
					masterControls,
					tr( "Master pitch" ) );
	m_masterPitchSpinBox->setModel( &m_song->m_masterPitchModel );
	ToolTip::add( m_masterPitchSpinBox, tr( "Master pitch" ) );
	masterControlsLayout->addWidget( m_masterPitchSpinBox, 1, 1, 1, 2,
					Qt::AlignBottom | Qt::AlignRight );
	
	m_masterPitchSpinBox->setZeroesVisible( false );
	m_masterPitchSpinBox->setAlignment( Qt::AlignRight );
	m_masterPitchSpinBox->setForceSign( true );
	
	frameLayout->addWidget( masterControls );
	
	// Create widget for visualization- and cpu-load-widget:
	QWidget * vc_w = new QWidget( tb );
	vc_w->setMaximumHeight( 41 );
	QVBoxLayout * vcw_layout = new QVBoxLayout( vc_w );
	vcw_layout->setMargin( 0 );
	vcw_layout->setSpacing( 0 );

	vcw_layout->addWidget( new SimpleVisualizationWidget( vc_w ), 0, Qt::AlignTop );
	vcw_layout->addWidget( new CPULoadWidget( vc_w ), 0, Qt::AlignBottom );

	frameLayout->addWidget( vc_w );
	
	// Put everything in a frame and place it in the toolbar:
	QFrame * frame = new QFrame( this );
	frame->setObjectName( "masterToolbarFrame" );
	frame->setFrameStyle(QFrame::Panel | QFrame::Plain);
	frame->setLineWidth(1);
	
	frame->setLayout( frameLayout );
	
	QHBoxLayout * mainLayout = new QHBoxLayout( this );
	mainLayout->addWidget( frame );
	mainLayout->setSpacing( 0 );
	mainLayout->setMargin( 0 );
	
	setContentsMargins(6, 6, 6, 6);
	setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
	
	gui->mainWindow()->addWidgetToToolBar( this );
}




MasterToolBar::~MasterToolBar()
{
}




void MasterToolBar::closeEvent( QCloseEvent * _ce )
{
	if( parentWidget() )
	{
		parentWidget()->hide();
	}
	else
	{
		hide();
	}
	_ce->ignore();
 }
