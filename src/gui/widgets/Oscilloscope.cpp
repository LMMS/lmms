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


#include <QMouseEvent>
#include <QPainter>

#include "Oscilloscope.h"
#include "GuiApplication.h"
#include "FontHelper.h"
#include "MainWindow.h"
#include "AudioEngine.h"
#include "Engine.h"
#include "Song.h"
#include "embed.h"

namespace lmms::gui
{


Oscilloscope::Oscilloscope( QWidget * _p ) :
	QWidget( _p ),
	m_background( embed::getIconPixmap( "output_graph" ) ),
	m_points( new QPointF[Engine::audioEngine()->framesPerPeriod()] ),
	m_active( false ),
	m_leftChannelColor(71, 253, 133),
	m_rightChannelColor(71, 253, 133),
	m_otherChannelsColor(71, 253, 133),
	m_clippingColor(255, 64, 64)
{
	setFixedSize( m_background.width(), m_background.height() );
	setActive( ConfigManager::inst()->value( "ui", "displaywaveform").toInt() );

	const fpp_t frames = Engine::audioEngine()->framesPerPeriod();
	m_buffer = new SampleFrame[frames];

	zeroSampleFrames(m_buffer, frames);


	setToolTip(tr("Oscilloscope"));
}




Oscilloscope::~Oscilloscope()
{
	delete[] m_buffer;
	delete[] m_points;
}




void Oscilloscope::updateAudioBuffer(const SampleFrame* buffer)
{
	if( !Engine::getSong()->isExporting() )
	{
		const fpp_t fpp = Engine::audioEngine()->framesPerPeriod();
		memcpy(m_buffer, buffer, sizeof(SampleFrame) * fpp);
	}
}




void Oscilloscope::setActive( bool _active )
{
	m_active = _active;
	if( m_active )
	{
		connect( getGUI()->mainWindow(),
					SIGNAL(periodicUpdate()),
					this, SLOT(update()));
		connect( Engine::audioEngine(),
			SIGNAL(nextAudioBuffer(const lmms::SampleFrame*)),
			this, SLOT(updateAudioBuffer(const lmms::SampleFrame*)));
	}
	else
	{
		disconnect( getGUI()->mainWindow(),
					SIGNAL(periodicUpdate()),
					this, SLOT(update()));
		disconnect( Engine::audioEngine(),
			SIGNAL(nextAudioBuffer(const lmms::SampleFrame*)),
			this, SLOT(updateAudioBuffer(const lmms::SampleFrame*)));
		// we have to update (remove last waves),
		// because timer doesn't do that anymore
		update();
	}
}


QColor const & Oscilloscope::leftChannelColor() const
{
	return m_leftChannelColor;
}

void Oscilloscope::setLeftChannelColor(QColor const & leftChannelColor)
{
	m_leftChannelColor = leftChannelColor;
}

QColor const & Oscilloscope::rightChannelColor() const
{
	return m_rightChannelColor;
}

void Oscilloscope::setRightChannelColor(QColor const & rightChannelColor)
{
	m_rightChannelColor = rightChannelColor;
}

QColor const & Oscilloscope::otherChannelsColor() const
{
	return m_otherChannelsColor;
}

void Oscilloscope::setOtherChannelsColor(QColor const & otherChannelsColor)
{
	m_otherChannelsColor = otherChannelsColor;
}

QColor const & Oscilloscope::clippingColor() const
{
	return m_clippingColor;
}

void Oscilloscope::setClippingColor(QColor const & clippingColor)
{
	m_clippingColor = clippingColor;
}


void Oscilloscope::paintEvent( QPaintEvent * )
{
	QPainter p( this );

	p.drawPixmap( 0, 0, m_background );

	if( m_active && !Engine::getSong()->isExporting() )
	{
		AudioEngine const * audioEngine = Engine::audioEngine();

		float masterOutput = audioEngine->masterGain();

		const fpp_t frames = audioEngine->framesPerPeriod();
		SampleFrame peakValues = getAbsPeakValues(m_buffer, frames);

		auto const leftChannelClips = clips(peakValues.left() * masterOutput);
		auto const rightChannelClips = clips(peakValues.right() * masterOutput);

		p.setRenderHint( QPainter::Antialiasing );

		// now draw all that stuff
		int w = width() - 4;
		const qreal xd = static_cast<qreal>(w) / frames;
		const qreal half_h = -(height() - 6) / 3.0 * static_cast<qreal>(masterOutput) - 1;
		int x_base = 2;
		const qreal y_base = height() / 2 - 0.5;

		qreal const width = 0.7;
		for( ch_cnt_t ch = 0; ch < DEFAULT_CHANNELS; ++ch )
		{
			QColor color = ch == 0 ? (leftChannelClips ? clippingColor() : leftChannelColor()) : // Check left channel
				ch == 1 ? (rightChannelClips ? clippingColor() : rightChannelColor()) : // Check right channel
				otherChannelsColor(); // Any other channel
			p.setPen(QPen(color, width));

			for (auto frame = std::size_t{0}; frame < frames; ++frame)
			{
				sample_t const clippedSample = AudioEngine::clip(m_buffer[frame][ch]);
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
		p.setFont(adjustedToPixelSize(p.font(), DEFAULT_FONT_SIZE));
		p.drawText( 6, height()-5, tr( "Click to enable" ) );
	}
}




void Oscilloscope::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton )
	{
		setActive( !m_active );
	}
}

bool Oscilloscope::clips(float level) const
{
	return level > 1.0f;
}


} // namespace lmms::gui
