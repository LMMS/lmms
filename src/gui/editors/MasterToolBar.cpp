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
	frameLayout->setSpacing( 16 );
	frameLayout->setContentsMargins( 12, 2, 12, 2 );
	
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
	
	QWidget * masterSliders = new QWidget( tb );
	QGridLayout * masterSliderLayout = new QGridLayout( masterSliders );
	masterSliderLayout->setSpacing( 0 );
	masterSliderLayout->setContentsMargins( 0, 0, 0, 0 );
	
	// Setup the master volume slider:
	QLabel * master_vol_lbl = new QLabel( tb );
	master_vol_lbl->setPixmap( embed::getIconPixmap( "master_volume" ) );

	m_masterVolumeSlider = new AutomatableSlider( tb,
							tr( "Master volume" ) );
	m_masterVolumeSlider->setModel( &m_song->m_masterVolumeModel );
	m_masterVolumeSlider->setOrientation( Qt::Horizontal );
	m_masterVolumeSlider->setPageStep( 1 );
	m_masterVolumeSlider->setTickPosition( QSlider::TicksLeft );
	m_masterVolumeSlider->setFixedSize( 100, 23 );
	m_masterVolumeSlider->setTickInterval( 50 );
	ToolTip::add( m_masterVolumeSlider, tr( "Master volume" ) );

	connect( m_masterVolumeSlider, SIGNAL( logicValueChanged( int ) ), this,
			SLOT( setMasterVolume( int ) ) );
	connect( m_masterVolumeSlider, SIGNAL( sliderPressed() ), this,
			SLOT( showMasterVolumeFloat()) );
	connect( m_masterVolumeSlider, SIGNAL( logicSliderMoved( int ) ), this,
			SLOT( updateMasterVolumeFloat( int ) ) );
	connect( m_masterVolumeSlider, SIGNAL( sliderReleased() ), this,
			SLOT( hideMasterVolumeFloat() ) );
	
	m_mvsStatus = new TextFloat;
	m_mvsStatus->setTitle( tr( "Master volume" ) );
	m_mvsStatus->setPixmap( embed::getIconPixmap( "master_volume" ) );

	masterSliderLayout->addWidget( master_vol_lbl, 1, 1, Qt::AlignVCenter );
	masterSliderLayout->addWidget( m_masterVolumeSlider, 1, 2 );
	
	
	// Setup the master pitch slider:
	QLabel * master_pitch_lbl = new QLabel( tb );
	master_pitch_lbl->setPixmap( embed::getIconPixmap( "master_pitch" ) );

	m_masterPitchSlider = new AutomatableSlider( tb, tr( "Master pitch" ) );
	m_masterPitchSlider->setModel( &m_song->m_masterPitchModel );
	m_masterPitchSlider->setOrientation( Qt::Horizontal );
	m_masterPitchSlider->setPageStep( 1 );
	m_masterPitchSlider->setTickPosition( QSlider::TicksLeft );
	m_masterPitchSlider->setFixedSize( 100, 23 );
	m_masterPitchSlider->setTickInterval( 12 );
	ToolTip::add( m_masterPitchSlider, tr( "Master pitch" ) );
	connect( m_masterPitchSlider, SIGNAL( logicValueChanged( int ) ), this,
			SLOT( setMasterPitch( int ) ) );
	connect( m_masterPitchSlider, SIGNAL( sliderPressed() ), this,
			SLOT( showMasterPitchFloat() ) );
	connect( m_masterPitchSlider, SIGNAL( logicSliderMoved( int ) ), this,
			SLOT( updateMasterPitchFloat( int ) ) );
	connect( m_masterPitchSlider, SIGNAL( sliderReleased() ), this,
			SLOT( hideMasterPitchFloat() ) );
	
	m_mpsStatus = new TextFloat;
	m_mpsStatus->setTitle( tr( "Master pitch" ) );
	m_mpsStatus->setPixmap( embed::getIconPixmap( "master_pitch" ) );

	masterSliderLayout->addWidget( master_pitch_lbl, 2, 1, Qt::AlignVCenter );
	masterSliderLayout->addWidget( m_masterPitchSlider, 2, 2 );
	
	frameLayout->addWidget( masterSliders );
	
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




void MasterToolBar::setMasterVolume( int new_val )
{
	updateMasterVolumeFloat( new_val );

	if( !m_mvsStatus->isVisible() && !m_song->m_loadingProject
					&& m_masterVolumeSlider->showStatus() )
	{
		m_mvsStatus->moveGlobal( m_masterVolumeSlider,
			QPoint( m_masterVolumeSlider->width() + 2, -2 ) );
		m_mvsStatus->setVisibilityTimeOut( 1000 );
	}
	Engine::mixer()->setMasterGain( new_val / 100.0f );
}




void MasterToolBar::showMasterVolumeFloat( void )
{
	m_mvsStatus->moveGlobal( m_masterVolumeSlider,
			QPoint( m_masterVolumeSlider->width() + 2, -2 ) );
	m_mvsStatus->show();
	updateMasterVolumeFloat( m_song->m_masterVolumeModel.value() );
}




void MasterToolBar::updateMasterVolumeFloat( int new_val )
{
	m_mvsStatus->setText( tr( "Value: %1%" ).arg( new_val ) );
}




void MasterToolBar::hideMasterVolumeFloat( void )
{
	m_mvsStatus->hide();
}




void MasterToolBar::setMasterPitch( int new_val )
{
	updateMasterPitchFloat( new_val );
	if( m_mpsStatus->isVisible() == false && m_song->m_loadingProject == false
					&& m_masterPitchSlider->showStatus() )
	{
		m_mpsStatus->moveGlobal( m_masterPitchSlider,
			QPoint( m_masterPitchSlider->width() + 2, -2 ) );
		m_mpsStatus->setVisibilityTimeOut( 1000 );
	}
}




void MasterToolBar::showMasterPitchFloat( void )
{
	m_mpsStatus->moveGlobal( m_masterPitchSlider,
			QPoint( m_masterPitchSlider->width() + 2, -2 ) );
	m_mpsStatus->show();
	updateMasterPitchFloat( m_song->m_masterPitchModel.value() );
}




void MasterToolBar::updateMasterPitchFloat( int new_val )
{
	m_mpsStatus->setText( tr( "Value: %1 semitones").arg( new_val ) );

}




void MasterToolBar::hideMasterPitchFloat( void )
{
	m_mpsStatus->hide();
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
