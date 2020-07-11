/*
 * SimpleVisualizationWidget.cpp - widget for visualization of sound-data
 *
 * Copyright (c) 2019 Lathigos <lathigos/at/tutanota.com>
 * Copyright (c) 2005-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QGridLayout>
#include <QLabel>

#include "SimpleVisualizationWidget.h"
#include "GuiApplication.h"
#include "gui_templates.h"
#include "MainWindow.h"
#include "Mixer.h"
#include "FxMixer.h"
#include "ProgressBar.h"
#include "Engine.h"
#include "ToolTip.h"
#include "Song.h"

#include "BufferManager.h"

#include "lmms_math.h"
#include "embed.h"


SimpleVisualizationWidget::SimpleVisualizationWidget( QWidget * _p ) :
	QWidget( _p )
{
	QGridLayout * mainLayout = new QGridLayout( this );
	mainLayout->setHorizontalSpacing( 4 );
	mainLayout->setVerticalSpacing( 2 );
	mainLayout->setContentsMargins( 0, 0, 0, 0 );
	
	QLabel * label = new QLabel( this );
	label->setObjectName( "integerDisplayTitle" );
	label->setText( tr( "OUT" ) );
	mainLayout->addWidget( label, 0, 0, 2, 1 );
	
	m_progressBarLeft = new ProgressBar( this,
					embed::getIconPixmap( "master_out_bg" ),
					embed::getIconPixmap( "master_out_leds" ) );
	mainLayout->addWidget( m_progressBarLeft, 0, 1, Qt::AlignBottom );
	
	m_progressBarRight = new ProgressBar( this,
					embed::getIconPixmap( "master_out_bg" ),
					embed::getIconPixmap( "master_out_leds" ) );
	mainLayout->addWidget( m_progressBarRight, 1, 1, Qt::AlignTop );

	ToolTip::add( this, tr( "Master volume output" ) );
	
	const fpp_t frames = Engine::mixer()->framesPerPeriod();
	m_buffer = new sampleFrame[frames];
	
	BufferManager::clear( m_buffer, frames );
	
	connect( gui->mainWindow(),
				SIGNAL( periodicUpdate() ),
				this, SLOT( updateVisualization() ) );
	connect( Engine::mixer(),
			SIGNAL( nextAudioBuffer( const surroundSampleFrame* ) ),
			this, SLOT( updateAudioBuffer( const surroundSampleFrame* ) ) );
}




SimpleVisualizationWidget::~SimpleVisualizationWidget()
{
	delete[] m_buffer;
}




void SimpleVisualizationWidget::updateAudioBuffer( const surroundSampleFrame * buffer )
{
	if( !Engine::getSong()->isExporting() )
	{
		FxMixer * m = Engine::fxMixer();
		
		// Add a slight persistence to the peak values to reduce flickering:
		if ((m_peakLeft < m->effectChannel(0)->m_peakLeft) ||
						(m_peakRight < m->effectChannel(0)->m_peakRight))
		{
			frameCounter = 0;
		}

		m_peakLeft = qMax(
						m_peakLeft,
						m->effectChannel(0)->m_peakLeft );

		m_peakRight = qMax(
						m_peakRight,
						m->effectChannel(0)->m_peakRight );
	}
}




void SimpleVisualizationWidget::updateVisualization()
{
	// Smoothen and display the display the peak values:
	const float fallOff = 1.25f;
	
	float const maxDB = 9.0f;
	float const minDB = ampToDbfs( 0.01f );
	
	float masterGain = Engine::mixer()->masterGain();
	
	// Render peaks as decibels:
	float const peakLeftDB = ampToDbfs(
					qMax<float>( minDB, m_peakLeft * masterGain ) );
	float const peakRightDB = ampToDbfs(
					qMax<float>( minDB, m_peakRight * masterGain ) );
	
	m_progressBarLeft->setValue( (peakLeftDB - minDB) / (maxDB - minDB) );
	m_progressBarRight->setValue( (peakRightDB - minDB) / (maxDB - minDB) );
	
	// Add a slight persistence to the peak values to reduce flickering:
	if (frameCounter > 4)
	{
		// Reset the peak values so that a new peak can be found:
		m_peakLeft = m_peakLeft / fallOff;
		m_peakRight = m_peakRight / fallOff;
	}
	else
	{
		frameCounter++;
	}
}




