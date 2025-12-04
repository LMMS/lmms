/*
* OscilloscopeGraph.cpp - Oscilloscope graph widget to handle drawing the waveform and user scrolling/zooming
*
* Copyright (c) 2025 Keratin
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

#include <functional>

namespace lmms::gui
{

OscilloscopeGraph::OscilloscopeGraph(QWidget* parent, OscilloscopeControls* controls):
	QWidget(parent),
	m_controls(controls),
	m_inputBufferReader(static_cast<Oscilloscope*>(m_controls->effect())->inputBuffer())
{
	setAutoFillBackground(true);
	setMinimumSize(400, 200);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	connect(getGUI()->mainWindow(), SIGNAL(periodicUpdate()), this, SLOT(update()));
	connect(Engine::audioEngine(), &AudioEngine::sampleRateChanged, this, &OscilloscopeGraph::changeSampleRate);
	changeSampleRate();
}

void OscilloscopeGraph::changeSampleRate()
{
		const auto newBufferSize = static_cast<std::size_t>(Engine::audioEngine()->outputSampleRate() * MaxBufferLengthSeconds);
	m_ringBuffer.resize(newBufferSize);
	m_ringBufferIndex = m_ringBufferIndex % newBufferSize;
}

void OscilloscopeGraph::paintEvent(QPaintEvent* pe)
{
	const auto effect = static_cast<Oscilloscope*>(m_controls->effect());

	// Update the ring buffer with any new data from the audio thread
	auto incomingBuffer = m_inputBufferReader.read_max(effect->inputBuffer().capacity());
	for (f_cnt_t f = 0; f < incomingBuffer.size(); ++f)
	{
		m_ringBuffer[m_ringBufferIndex] = incomingBuffer[f];
		m_ringBufferIndex = (m_ringBufferIndex + 1) % m_ringBuffer.size();
	}

	QPainter p(this);
	p.fillRect(0, 0, width(), height(), m_backgroundColor);

	const float amp = m_controls->m_ampModel.value() * 0.01f;
	int windowSizeFrames = m_ringBuffer.size()
		* m_controls->m_lengthModel.value()
		/ m_controls->m_lengthModel.maxValue();
	const int framesPerPixel = std::max(1, windowSizeFrames / width());

	int windowStartIndex = m_ringBufferIndex
		+ (m_ringBuffer.size() - windowSizeFrames)
		+ m_controls->m_phaseModel.value() * m_ringBuffer.size();
	// Quantize start index to prevent flickering (round up to prevent going below the current ring buffer index)
	const int frameOffsetWithinPixel = windowStartIndex % framesPerPixel;
	windowStartIndex += framesPerPixel - frameOffsetWithinPixel;
	// And subtract that amount from the window size to prevent going past off the right
	windowSizeFrames -= framesPerPixel - frameOffsetWithinPixel;

	p.setPen(m_minorLineColor);
	p.drawLine(0, height() / 2, width(), height() / 2);

	p.setPen(m_clippingLineColor);
	p.drawLine(0, height() / 2 * (1 - amp), width(), height() / 2 * (1 - amp));
	p.drawLine(0, height() / 2 * (1 + amp), width(), height() / 2 * (1 + amp));

	const bool hq = framesPerPixel > 1;
	const int xoffset = !hq * framesPerPixel;
	const float xscale = 1.f / (windowSizeFrames - framesPerPixel);

	auto drawWaveform = [&](QColor& color, auto&& getChannel)
	{
		p.setCompositionMode(QPainter::CompositionMode_Plus);
		p.setPen(color);
		for (int f = 0; f < windowSizeFrames - framesPerPixel; f += framesPerPixel)
		{
			const int currentIndex = (windowStartIndex + f) % m_ringBuffer.size();
			const int nextIndex = (windowStartIndex + f + framesPerPixel) % m_ringBuffer.size();

			float maxValue = getChannel(m_ringBuffer[hq ? currentIndex : nextIndex]);
			float minValue = getChannel(m_ringBuffer[currentIndex]);

			for (int i = currentIndex + 1; hq && i <= nextIndex; ++i)
			{
				maxValue = std::max(maxValue, getChannel(m_ringBuffer[i]));
				minValue = std::min(minValue, getChannel(m_ringBuffer[i]));
			}

			p.drawLine(
				width() * f * xscale,
				height() * (1 - minValue * amp) / 2,
				width() * (f + xoffset) * xscale,
				height() * (1 - maxValue * amp) / 2
			);
		}
	};

	if (!m_controls->m_stereoModel.value())
	{
		drawWaveform(m_monoColor, [](const SampleFrame& f){ return f.average(); });
	}
	else
	{
		drawWaveform(m_leftColor, [](const SampleFrame& f){ return f.left(); });
		drawWaveform(m_rightColor, [](const SampleFrame& f){ return f.right(); });
	}
}

void OscilloscopeGraph::wheelEvent(QWheelEvent* we)
{
	const float windowSizeMilliseconds = m_controls->m_lengthModel.value();
	const float mouseOffset = (1.0f - we->position().x() / width())
		* (windowSizeMilliseconds / m_controls->m_lengthModel.maxValue());
	const float zoomAmount = std::clamp(
		std::exp2(-we->angleDelta().y() / 240.0f),
		m_controls->m_lengthModel.minValue() / m_controls->m_lengthModel.value(),
		m_controls->m_lengthModel.maxValue() / m_controls->m_lengthModel.value()
	);

	if ((zoomAmount > 1.0f && windowSizeMilliseconds >= m_controls->m_lengthModel.maxValue()) || (zoomAmount < 1.0f && windowSizeMilliseconds <= m_controls->m_lengthModel.minValue())) { return; }

	const float newWindowSizeMilliseconds = windowSizeMilliseconds * zoomAmount;
	const float newPhase = m_controls->m_phaseModel.value() - mouseOffset * (1.0f - zoomAmount);
	m_controls->m_lengthModel.setValue(newWindowSizeMilliseconds);
	// Clamp to prevent the user from accidentally bringing the write position discontinuity into view
	m_controls->m_phaseModel.setValue(std::clamp(newPhase, newWindowSizeMilliseconds / m_controls->m_lengthModel.maxValue(), 1.0f));
}

void OscilloscopeGraph::mousePressEvent(QMouseEvent* me) { m_mousePos = me->x(); }

void OscilloscopeGraph::mouseMoveEvent(QMouseEvent* me)
{
	float newPhase = m_controls->m_phaseModel.value() + static_cast<float>(m_mousePos - me->x()) / width() * (m_controls->m_lengthModel.value() / m_controls->m_lengthModel.maxValue());
	m_controls->m_phaseModel.setValue(newPhase - std::floor(newPhase));
	m_mousePos = me->x();
}

} // namespace lmms::gui
