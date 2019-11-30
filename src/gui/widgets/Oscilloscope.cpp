/*
 * Oscilloscope.cpp
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


#include <QPainter>

#include "Oscilloscope.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "Mixer.h"
#include "Engine.h"
#include "ToolTip.h"
#include "Song.h"
#include "BufferManager.h"
#include "embed.h"


Oscilloscope::Oscilloscope( QWidget * _p ) :
	QWidget( _p ),
	m_background( embed::getIconPixmap( "output_graph" ) ),
	m_points( new QPointF[Engine::mixer()->framesPerPeriod()] ),
	m_color(71, 253, 133)
{
	setFixedSize( m_background.width(), m_background.height() );
	setAttribute( Qt::WA_OpaquePaintEvent, true );

	connect( gui->mainWindow(), SIGNAL( periodicUpdate() ), this, SLOT( update() ) );
	connect( Engine::mixer(), SIGNAL( nextAudioBuffer( const surroundSampleFrame* ) ),
		this, SLOT( updateAudioBuffer( const surroundSampleFrame* ) ) );

	const fpp_t frames = Engine::mixer()->framesPerPeriod();
	m_buffer = new sampleFrame[frames];

	BufferManager::clear( m_buffer, frames );


	ToolTip::add( this, tr( "Oscilloscope" ) );
}


Oscilloscope::~Oscilloscope()
{
	delete[] m_buffer;
	delete[] m_points;
}


void Oscilloscope::updateAudioBuffer( const surroundSampleFrame * buffer )
{
	if( !Engine::getSong()->isExporting() )
	{
		const fpp_t fpp = Engine::mixer()->framesPerPeriod();
		memcpy( m_buffer, buffer, sizeof( surroundSampleFrame ) * fpp );
	}
}


QColor const & Oscilloscope::getColor() const
{
	return m_color;
}


void Oscilloscope::setColor(QColor const & color)
{
	m_color = color;
}


void Oscilloscope::paintEvent( QPaintEvent * )
{
	QPainter p( this );

	p.drawPixmap( 0, 0, m_background );

	if( !Engine::getSong()->isExporting() )
	{
		Mixer const * mixer = Engine::mixer();

		float master_output = mixer->masterGain();

		const fpp_t frames = mixer->framesPerPeriod();

		p.setPen(QPen(m_color, 0.7));
		p.setRenderHint( QPainter::Antialiasing );

		// now draw all that stuff
		int w = width() - 4;
		const qreal xd = static_cast<qreal>(w) / frames;
		const qreal half_h = -( height() - 6 ) / 3.0 * static_cast<qreal>(master_output) - 1;
		int x_base = 2;
		const qreal y_base = height() / 2 - 0.5;

		for( ch_cnt_t ch = 0; ch < DEFAULT_CHANNELS; ++ch )
		{
			for( int frame = 0; frame < frames; ++frame )
			{
				sample_t const clippedSample = Mixer::clip(m_buffer[frame][ch]);
				m_points[frame] = QPointF(
					x_base + static_cast<qreal>(frame) * xd,
					y_base + ( static_cast<qreal>(clippedSample) * half_h ) );
			}
			p.drawPolyline( m_points, frames );
		}
	}
}
