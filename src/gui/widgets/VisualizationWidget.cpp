/*
 * VisualizationWidget.cpp - widget for visualization of sound-data
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


#include <QMouseEvent>
#include <QPainter>

#include "VisualizationWidget.h"
#include "GuiApplication.h"
#include "gui_templates.h"
#include "MainWindow.h"
#include "Mixer.h"
#include "Engine.h"
#include "ToolTip.h"
#include "Song.h"

#include "BufferManager.h"


VisualizationWidget::VisualizationWidget( const QPixmap & _bg, QWidget * _p,
						visualizationTypes _vtype ) :
	QWidget( _p ),
	s_background( _bg ),
	m_points( new QPointF[Engine::mixer()->framesPerPeriod()] ),
	m_active( false )
{
	setFixedSize( s_background.width(), s_background.height() );
	setAttribute( Qt::WA_OpaquePaintEvent, true );
	setActive( ConfigManager::inst()->value( "ui", "displaywaveform").toInt() );

	const fpp_t frames = Engine::mixer()->framesPerPeriod();
	m_buffer = new sampleFrame[frames];

	BufferManager::clear( m_buffer, frames );


	ToolTip::add( this, tr( "Oscilloscope" ) );
}




VisualizationWidget::~VisualizationWidget()
{
	delete[] m_buffer;
	delete[] m_points;
}




void VisualizationWidget::updateAudioBuffer( const surroundSampleFrame * buffer )
{
	if( !Engine::getSong()->isExporting() )
	{
		const fpp_t fpp = Engine::mixer()->framesPerPeriod();
		memcpy( m_buffer, buffer, sizeof( surroundSampleFrame ) * fpp );
	}
}




void VisualizationWidget::setActive( bool _active )
{
	m_active = _active;
	if( m_active )
	{
		connect( gui->mainWindow(),
					SIGNAL( periodicUpdate() ),
					this, SLOT( update() ) );
		connect( Engine::mixer(),
			SIGNAL( nextAudioBuffer( const surroundSampleFrame* ) ),
			this, SLOT( updateAudioBuffer( const surroundSampleFrame* ) ) );
	}
	else
	{
		disconnect( gui->mainWindow(),
					SIGNAL( periodicUpdate() ),
					this, SLOT( update() ) );
		disconnect( Engine::mixer(),
			SIGNAL( nextAudioBuffer( const surroundSampleFrame* ) ),
			this, SLOT( updateAudioBuffer( const surroundSampleFrame* ) ) );
		// we have to update (remove last waves),
		// because timer doesn't do that anymore
		update();
	}
}




void VisualizationWidget::paintEvent( QPaintEvent * )
{
	QPainter p( this );

	p.drawPixmap( 0, 0, s_background );

	if( m_active && !Engine::getSong()->isExporting() )
	{
		Mixer const * mixer = Engine::mixer();

		float master_output = mixer->masterGain();
		int w = width()-4;
		const float half_h = -( height() - 6 ) / 3.0 * master_output - 1;
		int x_base = 2;
		const float y_base = height()/2 - 0.5f;

//		p.setClipRect( 2, 2, w, height()-4 );


		const fpp_t frames = mixer->framesPerPeriod();
		Mixer::StereoSample peakValues = mixer->getPeakValues(m_buffer, frames);
		const float max_level = qMax<float>( peakValues.left, peakValues.right );

		// and set color according to that...
		if( max_level * master_output < 0.9 )
		{
			p.setPen( QColor( 71, 253, 133 ) );
		}
		else if( max_level * master_output < 1.0 )
		{
			p.setPen( QColor( 255, 192, 64 ) );
		}
		else
		{
			p.setPen( QColor( 255, 64, 64 ) );
		}

		p.setPen( QPen( p.pen().color(), 0.7 ) );

		const float xd = (float) w / frames;
		p.setRenderHint( QPainter::Antialiasing );

		// now draw all that stuff
		for( ch_cnt_t ch = 0; ch < DEFAULT_CHANNELS; ++ch )
		{
			for( int frame = 0; frame < frames; ++frame )
			{
				m_points[frame] = QPointF(
					x_base + (float) frame * xd,
					y_base + ( Mixer::clip(
						m_buffer[frame][ch] ) *
								half_h ) );
			}
			p.drawPolyline( m_points, frames );
		}
	}
	else
	{
		p.setPen( QColor( 192, 192, 192 ) );
		p.setFont( pointSize<7>( p.font() ) );
		p.drawText( 6, height()-5, tr( "Click to enable" ) );
	}
}




void VisualizationWidget::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton )
	{
		setActive( !m_active );
	}
}






