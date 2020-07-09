/* VectorView.h - declaration of VectorView class.
 *
 * Copyright (c) 2019 Martin Pavelek <he29/dot/HS/at/gmail/dot/com>
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

#include <QMouseEvent>
#include <QWheelEvent>
#include <QWidget>

#include "Knob.h"
#include "LedCheckbox.h"
#include "LocklessRingBuffer.h"
#include "VecControls.h"

//#define VEC_DEBUG


// Widget that displays a vectorscope visualization of stereo signal.
class VectorView : public QWidget
{
	Q_OBJECT
public:
	explicit VectorView(VecControls *controls, LocklessRingBuffer<sampleFrame> *inputBuffer, unsigned short displaySize, QWidget *parent = 0);
	virtual ~VectorView() {}

	QSize sizeHint() const override {return QSize(300, 300);}

protected:
	void paintEvent(QPaintEvent *event) override;
	void mouseDoubleClickEvent(QMouseEvent *event) override;
	void wheelEvent(QWheelEvent *event) override;

private slots:
	void periodicUpdate();

private:
	VecControls *m_controls;

	LocklessRingBuffer<sampleFrame> *m_inputBuffer;
	LocklessRingBufferReader<sampleFrame> m_bufferReader;

	std::vector<uchar> m_displayBuffer;
	const unsigned short m_displaySize;

	bool m_visible;

	float m_zoom;

	// State variables for comparison with previous repaint
	unsigned int m_persistTimestamp;
	unsigned int m_zoomTimestamp;
	bool m_oldHQ;
	int m_oldX;
	int m_oldY;

#ifdef VEC_DEBUG
	float m_executionAvg = 0;
#endif
};
#endif // VECTORVIEW_H
