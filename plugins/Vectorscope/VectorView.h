/* VectorView.h - declaration of VectorView class.
 *
 * Copyright (c) 2019 Martin Pavelek <he29/dot/HS/at/gmail/dot/com>
 * Copyright (c) 2025- Michael Gregorius
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
#ifndef VECTORVIEW_H
#define VECTORVIEW_H

#include <QWidget>

#include "LocklessRingBuffer.h"

namespace lmms
{
class VecControls;
class SampleFrame;
}

//#define VEC_DEBUG

namespace lmms::gui
{


// Widget that displays a vectorscope visualization of stereo signal.
class VectorView : public QWidget
{
	Q_OBJECT
public:
	VectorView(VecControls* controls, LocklessRingBuffer<SampleFrame>* inputBuffer, QWidget* parent = nullptr);
	~VectorView() override = default;

	QSize sizeHint() const override {return QSize(300, 300);}

	Q_PROPERTY(QColor colorTrace MEMBER m_colorTrace)
	Q_PROPERTY(QColor colorGrid MEMBER m_colorGrid)
	Q_PROPERTY(QColor colorLabels MEMBER m_colorLabels)

protected:
	void paintEvent(QPaintEvent *event) override;
	void mouseDoubleClickEvent(QMouseEvent *event) override;
	void wheelEvent(QWheelEvent *event) override;

private slots:
	void periodicUpdate();

private:
	void drawZoomInfo();

private:
	VecControls *m_controls;

	LocklessRingBuffer<SampleFrame> *m_inputBuffer;
	LocklessRingBufferReader<SampleFrame> m_bufferReader;

	float m_zoom;

	// State variables for comparison with previous repaint
	unsigned int m_zoomTimestamp;

	QPointF m_lastPoint = QPoint();

	QColor m_colorTrace = QColor(60, 255, 130, 255);	// ~LMMS green
	QColor m_colorGrid = QColor(76, 80, 84, 128);		// ~60 % gray (slightly cold / blue), 50 % transparent
	QColor m_colorLabels = QColor(76, 80, 84, 255);		// ~60 % gray (slightly cold / blue)

#ifdef VEC_DEBUG
	float m_executionAvg = 0;
#endif
};


} // namespace lmms::gui

#endif // VECTORVIEW_H
