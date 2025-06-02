/*
* OscilloscopeGraph.cpp - Example effect gui boilerplate code
*
* Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
* Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "OscilloscopeGraph.h"
#include "OscilloscopeControls.h"
#include "Oscilloscope.h"
#include "embed.h"
#include "GuiApplication.h"
#include "MainWindow.h"

#include <QPainter>
#include <QSizePolicy>

namespace lmms::gui
{

OscilloscopeGraph::OscilloscopeGraph(QWidget* parent, OscilloscopeControls* controls):
	QWidget(parent),
	m_controls(controls)
{
	setAutoFillBackground(true);
	connect(getGUI()->mainWindow(), SIGNAL(periodicUpdate()), this, SLOT(update()));
	setMinimumSize(200, 100);
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
}

void OscilloscopeGraph::paintEvent(QPaintEvent* pe)
{
	if (!isVisible()) { return; }

	QPainter p(this);
	p.fillRect(0, 0, width(), height(), QColor(0,0,0));
	p.setPen(QColor(255, 255, 255));

	OscilloscopeEffect* effect = static_cast<OscilloscopeEffect*>(m_controls->effect());

	float amp = m_controls->amplitude();
	int period = std::max(1, static_cast<int>(effect->estimatedPeriod()));

	int bufferSize = effect->bufferSize();
	int visibleBufferSize = m_controls->length();
	SampleFrame* buffer = effect->buffer();

	int bufferIndex = effect->bufferIndex();
	int elapsedFrames = effect->elapsedFrames();
	int bufferIndexStartOfPeriod = (bufferIndex - elapsedFrames % period - visibleBufferSize) % bufferSize;

	p.setPen(QColor(100, 100, 100));
	p.drawLine(0, height() / 2, width(), height() / 2);
	p.setPen(QColor(100, 0, 0));

	p.setPen(QColor(255, 255, 255));
	for (int f = 1; f < visibleBufferSize; ++f)
	{
		// Adding bufferSize to prevent negative modulus
		int currentIndex = (bufferIndexStartOfPeriod + f + bufferSize) % bufferSize;
		int previousIndex = (bufferIndexStartOfPeriod + f - 1 + bufferSize) % bufferSize;
		p.drawLine(
			width() * (f - 1) / visibleBufferSize,
			height() * (1 + buffer[previousIndex].average() * amp) / 2,
			width() * f / visibleBufferSize,
			height() * (1 + buffer[currentIndex].average() * amp) / 2
		);
	}
}


} // namespace lmms::gui
