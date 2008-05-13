#ifndef SINGLE_SOURCE_COMPILE

/*
 * visualization_widget.cpp - widget for visualization of sound-data
 *
 * Copyright (c) 2005-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QtCore/QTimer>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>

#include "visualization_widget.h"
#include "embed.h"
#include "engine.h"
#include "templates.h"
#include "tooltip.h"
#include "song.h"
#include "song_editor.h"



visualizationWidget::visualizationWidget( const QPixmap & _bg, QWidget * _p,
						visualizationTypes _vtype ) :
	QWidget( _p ),
	s_background( _bg ),
	m_active( FALSE )
{
	setFixedSize( s_background.width(), s_background.height() );


	const fpp_t frames = engine::getMixer()->framesPerPeriod();
	m_buffer = new sampleFrame[frames];

	engine::getMixer()->clearAudioBuffer( m_buffer, frames );


	toolTip::add( this, tr( "click to enable/disable visualization of "
							"master-output" ) );

	setActive( TRUE );

}




visualizationWidget::~visualizationWidget()
{
	delete[] m_buffer;
}




void visualizationWidget::updateAudioBuffer( void )
{
	if( !engine::getSong()->isExporting() )
	{
		engine::getMixer()->lock();
		const surroundSampleFrame * c = engine::getMixer()->
							currentReadBuffer();
		for( f_cnt_t f = 0; f < engine::getMixer()->framesPerPeriod();
									++f )
		{
			m_buffer[f][0] = c[f][0];
			m_buffer[f][1] = c[f][1];
		}
		engine::getMixer()->unlock();
	}
}




void visualizationWidget::setActive( bool _active )
{
	m_active = _active;
	if( m_active )
	{
		connect( engine::getSongEditor(),
					SIGNAL( periodicUpdate() ),
					this, SLOT( update() ) );
		connect( engine::getMixer(),
					SIGNAL( nextAudioBuffer() ),
				this, SLOT( updateAudioBuffer() ) );
	}
	else
	{
		disconnect( engine::getSongEditor(),
					SIGNAL( periodicUpdate() ),
					this, SLOT( update() ) );
		disconnect( engine::getMixer(),
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
		float master_output = engine::getMixer()->masterGain();
		int w = width()-4;
		float half_h = -( height() - 6 ) / 3.0 * master_output - 1;
		int x_base = 2;
		int y_base = height()/2 - 1;

//		p.setClipRect( 2, 2, w, height()-4 );


		const fpp_t frames =
				engine::getMixer()->framesPerPeriod();
		const float max_level = qMax<float>(
				mixer::peakValueLeft( m_buffer, frames ),
				mixer::peakValueRight( m_buffer, frames ) );

		// and set color according to that...
		if( max_level * master_output < 0.9 )
		{
			p.setPen( QColor( 96, 255, 96 ) );
		}
		else if( max_level * master_output < 1.0 )
		{
			p.setPen( QColor( 255, 192, 64 ) );
		}
		else
		{
			p.setPen( QColor( 255, 64, 64 ) );
		}

		const int xd = w / frames;

		// now draw all that stuff
		for( ch_cnt_t ch = 0; ch < DEFAULT_CHANNELS; ++ch )
		{
			int old_y = y_base +
				(int)( mixer::clip( m_buffer[0][ch] )*half_h );
			for( fpp_t frame = 0; frame < frames; ++frame )
			{
				int cur_y = y_base + (int)(
					mixer::clip( m_buffer[frame][ch] ) *
								half_h );
				const int xp = x_base + frame * w / frames;
				p.drawLine( xp, old_y, xp+xd, cur_y );
				old_y = cur_y;
			}
		}

	}
}




void visualizationWidget::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton )
	{
		setActive( !m_active );
	}
}



#include "visualization_widget.moc"


#endif
