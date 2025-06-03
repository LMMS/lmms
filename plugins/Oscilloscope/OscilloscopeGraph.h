/*
* OscilloscopeGraph.h - Oscilloscope graph widget to handle drawing the waveform and user scrolling/zooming
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

#ifndef LMMS_GUI_OSCILLOSCOPE_GRAPH_H
#define LMMS_GUI_OSCILLOSCOPE_GRAPH_H

#include "OscilloscopeControls.h"
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
	Q_PROPERTY(QColor monoColor READ monoColor WRITE setMonoColor)
	Q_PROPERTY(QColor leftColor READ leftColor WRITE setLeftColor)
	Q_PROPERTY(QColor rightColor READ rightColor WRITE setRightColor)
	Q_PROPERTY(QColor minorLineColor READ minorLineColor WRITE setMinorLineColor)
	Q_PROPERTY(QColor clippingLineColor READ clippingLineColor WRITE setClippingLineColor)
	Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor)
public:
	OscilloscopeGraph(QWidget* parent, OscilloscopeControls* controls);
	~OscilloscopeGraph() override = default;

	void paintEvent(QPaintEvent* pe) override;
	void wheelEvent(QWheelEvent* we) override;
	void mousePressEvent(QMouseEvent* me) override;
	void mouseReleaseEvent(QMouseEvent* me) override;
	void mouseMoveEvent(QMouseEvent* me) override;

	QColor monoColor() const { return m_monoColor; }
	void setMonoColor(QColor c) { m_monoColor = c; }
	QColor leftColor() const { return m_leftColor; }
	void setLeftColor(QColor c) { m_leftColor = c; }
	QColor rightColor() const { return m_rightColor; }
	void setRightColor(QColor c) { m_rightColor = c; }
	QColor minorLineColor() const { return m_minorLineColor; }
	void setMinorLineColor(QColor c) { m_minorLineColor = c; }
	QColor clippingLineColor() const { return m_clippingLineColor; }
	void setClippingLineColor(QColor c) { m_clippingLineColor = c; }
	QColor backgroundColor() const { return m_backgroundColor; }
	void setBackgroundColor(QColor c) { m_backgroundColor = c; }
private:
	OscilloscopeControls* m_controls;
	int m_mousePos;
	bool m_mousePressed = false;

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
