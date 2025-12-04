/*
* OscilloscopeGraph.h - Oscilloscope graph widget to handle drawing the waveform and user scrolling/zooming
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

#ifndef LMMS_GUI_OSCILLOSCOPE_GRAPH_H
#define LMMS_GUI_OSCILLOSCOPE_GRAPH_H

#include "OscilloscopeControls.h"
#include "LocklessRingBuffer.h"
#include <QWidget>

namespace lmms
{

class OscilloscopeControls;
class FloatModel;

namespace gui
{

class Knob;

class OscilloscopeGraph : public QWidget
{
	Q_OBJECT
	Q_PROPERTY(QColor monoColor MEMBER m_monoColor)
	Q_PROPERTY(QColor leftColor MEMBER m_leftColor)
	Q_PROPERTY(QColor rightColor MEMBER m_rightColor)
	Q_PROPERTY(QColor minorLineColor MEMBER m_minorLineColor)
	Q_PROPERTY(QColor clippingLineColor MEMBER m_clippingLineColor)
	Q_PROPERTY(QColor backgroundColor MEMBER m_backgroundColor)
public:
	OscilloscopeGraph(QWidget* parent, OscilloscopeControls* controls);
	~OscilloscopeGraph() override = default;

	void paintEvent(QPaintEvent* pe) override;
	void wheelEvent(QWheelEvent* we) override;
	void mousePressEvent(QMouseEvent* me) override;
	void mouseMoveEvent(QMouseEvent* me) override;

	//! Maximum length of the oscilloscope history in seconds
	static constexpr float MaxBufferLengthSeconds = 5.0f;

private slots:
	void changeSampleRate();

private:
	OscilloscopeControls* m_controls;

	LocklessRingBufferReader<SampleFrame> m_inputBufferReader;

	// Buffer which is actually drawn on the screen, not the ring buffer used for communication between threads
	std::vector<SampleFrame> m_ringBuffer = {};
	int m_ringBufferIndex = 0;

	int m_mousePos;

	QColor m_monoColor = QColor(255, 255, 255);
	QColor m_leftColor = QColor(126, 146, 255, 128);
	QColor m_rightColor = QColor(255, 128, 128, 128);
	QColor m_minorLineColor = QColor(100, 100, 100);
	QColor m_clippingLineColor = QColor(100, 0, 0);
	QColor m_backgroundColor = QColor(0, 0, 0);
};

} // namespace gui

} // namespace lmms

#endif // LMMS_GUI_OSCILLOSCOPE_GRAPH_H
