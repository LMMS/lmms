/*
* OscilloscopeGraph.cpp - Oscilloscope graph widget to handle drawing the waveform and user scrolling/zooming
*
* Copyright (c) 2025 Keratin
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
#include <QWheelEvent>

namespace lmms::gui
{

OscilloscopeGraph::OscilloscopeGraph(QWidget* parent, OscilloscopeControls* controls):
	QWidget(parent),
	m_controls(controls)
{
	setAutoFillBackground(true);
	connect(getGUI()->mainWindow(), SIGNAL(periodicUpdate()), this, SLOT(update()));
	setMinimumSize(400, 200);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void OscilloscopeGraph::paintEvent(QPaintEvent* pe)
{
	if (!isVisible()) { return; }

	QPainter p(this);
	p.fillRect(0, 0, width(), height(), m_backgroundColor);

	Oscilloscope* effect = static_cast<Oscilloscope*>(m_controls->effect());

	float amp = m_controls->m_ampModel.value() * 0.01f;
	float phase = m_controls->m_phaseModel.value();
	int windowSize = m_controls->m_lengthModel.value();
	int framesPerPixel = std::max(1, windowSize / width());


	SampleFrame* buffer = effect->buffer();
	int bufferSize = Oscilloscope::BUFFER_SIZE;
	int bufferIndex = effect->bufferIndex();
	int windowStartIndex = bufferIndex + (bufferSize - windowSize) + (phase) * bufferSize;
	// Quantize start index to prevent flickering
	windowStartIndex -= windowStartIndex % framesPerPixel;

	p.setPen(m_minorLineColor);
	p.drawLine(0, height() / 2, width(), height() / 2);

	p.setPen(m_clippingLineColor);
	p.drawLine(0, height() / 2 * (1 - amp), width(), height() / 2 * (1 - amp));
	p.drawLine(0, height() / 2 * (1 + amp), width(), height() / 2 * (1 + amp));

	const bool hq = framesPerPixel > 1;
	const int xoffset = !hq * framesPerPixel;
	const float xscale = 1.f / (windowSize - framesPerPixel);
	const bool stereo = m_controls->m_stereoModel.value();
	if (!stereo)
	{
		p.setPen(m_monoColor);
		for (int f = 0; f < windowSize - framesPerPixel; f += framesPerPixel)
		{
			const int currentIndex = (windowStartIndex + f) % bufferSize;
			const int nextIndex = (windowStartIndex + f + framesPerPixel) % bufferSize;
			float maxValue = buffer[hq ? currentIndex : nextIndex].average();
			float minValue = buffer[currentIndex].average();
			for (int i = currentIndex + 1; hq && i <= nextIndex; ++i)
			{
				maxValue = std::max(maxValue, buffer[i].average());
				minValue = std::min(minValue, buffer[i].average());
			}
			p.drawLine(
				width() * f * xscale,
				height() * (1 - minValue * amp) / 2,
				width() * (f + xoffset) * xscale,
				height() * (1 - maxValue * amp) / 2
			);
		}
	}
	else
	{
		p.setPen(m_leftColor);
		for (int f = 0; f < windowSize - framesPerPixel; f += framesPerPixel)
		{
			const int currentIndex = (windowStartIndex + f) % bufferSize;
			const int nextIndex = (windowStartIndex + f + framesPerPixel) % bufferSize;
			float maxValue = buffer[hq ? currentIndex : nextIndex].left();
			float minValue = buffer[currentIndex].left();
			for (int i = currentIndex + 1; hq && i <= nextIndex; ++i)
			{
				maxValue = std::max(maxValue, buffer[i].left());
				minValue = std::min(minValue, buffer[i].left());
			}
			p.drawLine(
				width() * f * xscale,
				height() * (1 - minValue * amp) / 2,
				width() * (f + xoffset) * xscale,
				height() * (1 - maxValue * amp) / 2
			);
		}
		p.setPen(m_rightColor);
		for (int f = 0; f < windowSize - framesPerPixel; f += framesPerPixel)
		{
			const int currentIndex = (windowStartIndex + f) % bufferSize;
			const int nextIndex = (windowStartIndex + f + framesPerPixel) % bufferSize;
			float maxValue = buffer[hq ? currentIndex : nextIndex].right();
			float minValue = buffer[currentIndex].right();
			for (int i = currentIndex + 1; hq && i <= nextIndex; ++i)
			{
				maxValue = std::max(maxValue, buffer[i].right());
				minValue = std::min(minValue, buffer[i].right());
			}
			p.drawLine(
				width() * f * xscale,
				height() * (1 - minValue * amp) / 2,
				width() * (f + xoffset) * xscale,
				height() * (1 - maxValue * amp) / 2
			);
		}
	}
}

void OscilloscopeGraph::wheelEvent(QWheelEvent* we)
{
	int windowSize = m_controls->m_lengthModel.value();
	float phase = m_controls->m_phaseModel.value();
	float mouseOffset = (1.0f - we->position().x() / width()) * windowSize / Oscilloscope::BUFFER_SIZE;
	float zoomAmount = std::clamp(std::exp2(-we->angleDelta().y() / 240.0f), m_controls->m_lengthModel.minValue() / windowSize, m_controls->m_lengthModel.maxValue() / windowSize);

	if ((zoomAmount > 1.0f && windowSize >= Oscilloscope::BUFFER_SIZE) || (zoomAmount < 1.0f && windowSize <= 10)) { return; }

	int newWindowSize = windowSize * zoomAmount;
	float newPhase = phase - mouseOffset * (1.0f - zoomAmount);
	m_controls->m_lengthModel.setValue(newWindowSize);
	m_controls->m_phaseModel.setValue(newPhase - std::floor(newPhase));
}

void OscilloscopeGraph::mousePressEvent(QMouseEvent* me)
{
	m_mousePressed = true;
	m_mousePos = me->x();
}
void OscilloscopeGraph::mouseReleaseEvent(QMouseEvent* me)
{
	m_mousePressed = false;
}

void OscilloscopeGraph::mouseMoveEvent(QMouseEvent* me)
{
	float phase = m_controls->m_phaseModel.value();
	int windowSize = m_controls->m_lengthModel.value();
	float newPhase = phase + 1.0f * (m_mousePos - me->x()) / width() * windowSize / Oscilloscope::BUFFER_SIZE;
	m_controls->m_phaseModel.setValue(newPhase - std::floor(newPhase));
	m_mousePos = me->x();
}

} // namespace lmms::gui
