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
	m_active( false ),
	m_normalColor(71, 253, 133),
	m_warningColor(255, 192, 64),
	m_clippingColor(255, 64, 64)
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


QColor const & VisualizationWidget::normalColor() const
{
	return m_normalColor;
}

void VisualizationWidget::setNormalColor(QColor const & normalColor)
{
	m_normalColor = normalColor;
}

QColor const & VisualizationWidget::warningColor() const
{
	return m_warningColor;
}

void VisualizationWidget::setWarningColor(QColor const & warningColor)
{
	m_warningColor = warningColor;
}

QColor const & VisualizationWidget::clippingColor() const
{
	return m_clippingColor;
}

void VisualizationWidget::setClippingColor(QColor const & clippingColor)
{
	m_clippingColor = clippingColor;
}


void VisualizationWidget::paintEvent( QPaintEvent * )
{
	QPainter p( this );

	p.drawPixmap( 0, 0, s_background );

	if( m_active && !Engine::getSong()->isExporting() )
	{
		Mixer const * mixer = Engine::mixer();

		float master_output = mixer->masterGain();

		const fpp_t frames = mixer->framesPerPeriod();
		Mixer::StereoSample peakValues = mixer->getPeakValues(m_buffer, frames);
		const float max_level = qMax<float>( peakValues.left, peakValues.right );

		// Set the color of the line according to the maximum level
		float const maxLevelWithAppliedMasterGain = max_level * master_output;
		p.setPen(QPen(determineLineColor(maxLevelWithAppliedMasterGain), 0.7));

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


QColor const & VisualizationWidget::determineLineColor(float level) const
{
	if( level < 0.9f )
	{
		return normalColor();
	}
	else if( level <= 1.0f )
	{
		return warningColor();
	}
	else
	{
		return clippingColor();
	}
}



