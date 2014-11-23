/*
 * visualization_widget.cpp - widget for visualization of sound-data
 *
 * Copyright (c) 2005-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>

#include "visualization_widget.h"
#include "gui_templates.h"
#include "MainWindow.h"
#include "embed.h"
#include "engine.h"
#include "tooltip.h"
#include "song.h"

#include "config_mgr.h"



visualizationWidget::visualizationWidget( const QPixmap & _bg, QWidget * _p,
						visualizationTypes _vtype ) :
	QWidget( _p ),
	s_background( _bg ),
	m_points( new QPointF[engine::mixer()->framesPerPeriod()] ),
	m_active( false )
{
	setFixedSize( s_background.width(), s_background.height() );
	setAttribute( Qt::WA_OpaquePaintEvent, true );
	setActive( configManager::inst()->value( "ui", "displaywaveform").toInt() );

	const fpp_t frames = engine::mixer()->framesPerPeriod();
	m_buffer = new sampleFrame[frames];

	engine::mixer()->clearAudioBuffer( m_buffer, frames );


	toolTip::add( this, tr( "click to enable/disable visualization of "
							"master-output" ) );
}




visualizationWidget::~visualizationWidget()
{
	delete[] m_buffer;
	delete[] m_points;
}




void visualizationWidget::updateAudioBuffer()
{
	if( !engine::getSong()->isExporting() )
	{
		engine::mixer()->lock();
		const surroundSampleFrame * c = engine::mixer()->
							currentReadBuffer();
		const fpp_t fpp = engine::mixer()->framesPerPeriod();
		memcpy( m_buffer, c, sizeof( surroundSampleFrame ) * fpp );
		engine::mixer()->unlock();
	}
}




void visualizationWidget::setActive( bool _active )
{
	m_active = _active;
	if( m_active )
	{
		connect( engine::mainWindow(),
					SIGNAL( periodicUpdate() ),
					this, SLOT( update() ) );
		connect( engine::mixer(),
					SIGNAL( nextAudioBuffer() ),
				this, SLOT( updateAudioBuffer() ) );
	}
	else
	{
		disconnect( engine::mainWindow(),
					SIGNAL( periodicUpdate() ),
					this, SLOT( update() ) );
		disconnect( engine::mixer(),
					SIGNAL( nextAudioBuffer() ),
				this, SLOT( updateAudioBuffer() ) );
		// we have to update (remove last waves),
		// because timer doesn't do that anymore
		update();
	}
}




void visualizationWidget::paintEvent( QPaintEvent * )
{
	QPainter p( this );

	p.drawPixmap( 0, 0, s_background );

	if( m_active && !engine::getSong()->isExporting() )
	{
		float master_output = engine::mixer()->masterGain();
		int w = width()-4;
		const float half_h = -( height() - 6 ) / 3.0 * master_output - 1;
		int x_base = 2;
		const float y_base = height()/2 - 0.5f;

//		p.setClipRect( 2, 2, w, height()-4 );


		const fpp_t frames =
				engine::mixer()->framesPerPeriod();
		const float max_level = qMax<float>(
				Mixer::peakValueLeft( m_buffer, frames ),
				Mixer::peakValueRight( m_buffer, frames ) );

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




void visualizationWidget::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton )
	{
		setActive( !m_active );
	}
}



#include "moc_visualization_widget.cxx"


