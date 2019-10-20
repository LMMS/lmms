/*
 * SimpleVisualizationWidget.cpp - widget for visualization of sound-data
 *
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
		const fpp_t fpp = Engine::mixer()->framesPerPeriod();
		memcpy( m_buffer, buffer, sizeof( surroundSampleFrame ) * fpp );
	}
}




void SimpleVisualizationWidget::updateVisualization()
{
	const fpp_t fpp = Engine::mixer()->framesPerPeriod();
	Mixer::StereoSample peakSamples = Engine::mixer()->getPeakValues(m_buffer, fpp);
	
	const float fallOff = 0.90;
	
	previousPeakLeft = qMax( peakSamples.left, previousPeakLeft * fallOff );
	previousPeakRight = qMax( peakSamples.right, previousPeakRight * fallOff );
	
	m_progressBarLeft->setValue( previousPeakLeft );
	m_progressBarRight->setValue( previousPeakRight );
	
	
}



