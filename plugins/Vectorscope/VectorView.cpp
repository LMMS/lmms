/* VectorViewView.cpp - implementation of VectorViewView class.
 *
 * Copyright (c) 2019 Martin Pavelek <he29/dot/HS/at/gmail/dot/com>
 *
 * This file is part of LMMS - https://lmms.io
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#include "VectorView.h"

#include <cmath>
#include <QImage>
#include <QPainter>

#include "GuiApplication.h"
#include "MainWindow.h"

#define VEC_DEBUG
#include <iostream>

VectorView::VectorView(VecControls *controls, LocklessRingBuffer<sampleFrame> *inputBuffer, unsigned short displaySize, QWidget *_parent) :
	QWidget(_parent),
	m_controls(controls),
	m_inputBuffer(inputBuffer),
	m_bufferReader(*inputBuffer),
	m_displaySize(displaySize)
{
	setMinimumSize(128, 128);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	connect(gui->mainWindow(), SIGNAL(periodicUpdate()), this, SLOT(periodicUpdate()));

	m_displayBuffer.resize(sizeof qRgb(0,0,0) * m_displaySize * m_displaySize, 0);

	#ifdef SA_DEBUG
		m_execution_avg = 0;
	#endif
}


// Compose and draw all the content; called by Qt.
void VectorView::paintEvent(QPaintEvent *event)
{
	#ifdef VEC_DEBUG
		unsigned int draw_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	#endif

	// all drawing done in this method, local variables are sufficient for the boundary
	const int displayTop = 1;
	const int displayBottom = height() -2;
	const int displayLeft = 1;
	const int displayRight = width() -2;
	const int displayWidth = displayRight - displayLeft;
	const int displayHeight = displayBottom - displayTop;

	const int centerX = displayLeft + (displayWidth / 2) + 1;
	const int centerY = displayTop + (displayHeight / 2) + 1;

	float label_width = 20;
	float label_height = 16;
	float margin = 2;

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, true);

	// dim stored image based on persistence and elapsed time
		// check timestamp, limit dimming to 10 FPS
	const float persist = m_controls->m_persistenceModel.value();
	for (std::size_t i = 0; i < m_displayBuffer.size(); i++)
	{
		m_displayBuffer.data()[i] = m_displayBuffer.data()[i] * persist;
	}
	// get input data using a lockless FIFO buffer
	auto in_buffer = m_bufferReader.read_max(m_inputBuffer->capacity());
	std::size_t frame_count = in_buffer.size();


	//todo: idealne by to chtelo nejakou drawing function, old + new + pocet pixelu mezi nima
		// mozna i podle vzdalenosti (tj. rychlosti) tlumit intenzitu drahy, to by byl ultimatni simulacni rezim

	unsigned int x = m_displaySize / 2;
	unsigned int y = m_displaySize / 2;
	// draw new points on top
	for (; frame_count; frame_count--)	// twice smaller
	{
		float left = in_buffer[frame_count][0] * (m_displaySize -1) * 0.25;
		float right = in_buffer[frame_count][1] * (m_displaySize -1) * 0.25;

		x = fmax(fmin((3*x_old + (right - left + m_displaySize / 2)) / 4, m_displaySize - 1), 0);
		y = fmax(fmin((3*y_old + (m_displaySize - (right + left + m_displaySize / 2))) / 4, m_displaySize - 1), 0);
		((QRgb*)m_displayBuffer.data())[x + y * m_displaySize] = m_controls->m_colorFG.rgb();

		x = fmax(fmin((x_old + (right - left + m_displaySize / 2)) / 2, m_displaySize - 1), 0);
		y = fmax(fmin((y_old + (m_displaySize - (right + left + m_displaySize / 2))) / 2, m_displaySize - 1), 0);
		((QRgb*)m_displayBuffer.data())[x + y * m_displaySize] = m_controls->m_colorFG.rgb();

		x = fmax(fmin((x_old + 3*(right - left + m_displaySize / 2)) / 4, m_displaySize - 1), 0);
		y = fmax(fmin((y_old + 3*(m_displaySize - (right + left + m_displaySize / 2))) / 4, m_displaySize - 1), 0);
		((QRgb*)m_displayBuffer.data())[x + y * m_displaySize] = m_controls->m_colorFG.rgb();

		x = fmax(fmin(right - left + m_displaySize / 2, m_displaySize - 1), 0);
		y = fmax(fmin(m_displaySize - (right + left + m_displaySize / 2), m_displaySize - 1), 0);
		x_old = x;
		y_old = y;
//		std::cout << "point " << frame_count << " left " << left << " right " << right << std::endl;

		((QRgb*)m_displayBuffer.data())[x + y * m_displaySize] = m_controls->m_colorFG.rgb();
	}

	// draw the final image
	QImage temp = QImage(m_displayBuffer.data(),	// raw pixel data to display
						 m_displaySize,
						 m_displaySize,
						 QImage::Format_RGB32);
	temp.setDevicePixelRatio(devicePixelRatio());
	painter.drawImage(displayLeft, displayTop,
					  temp.scaled(displayWidth * devicePixelRatio(),
								  displayHeight * devicePixelRatio(),
								  Qt::IgnoreAspectRatio,
								  Qt::SmoothTransformation));

	// draw the grid
	painter.setPen(QPen(m_controls->m_colorGrid, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	painter.drawEllipse(QPoint(centerX, centerY), 8, 8);
	painter.drawEllipse(QPoint(centerX, centerY), displayWidth/2, displayWidth/2);

	// draw the outline
//	painter.setPen(QPen(m_controls->m_colorGrid, 2, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
//	painter.drawRoundedRect(displayLeft, displayTop, displayWidth, displayBottom, 2.0, 2.0);

	#ifdef VEC_DEBUG
		//init: 40/10; disable print: 11.5/2.2; no dimming: 10/0.6; no addition: no difference
		// → qpainter: 10/0.6	dimming 1.5/1.5		
		// display what FPS would be achieved if vectorscope ran in a loop
		draw_time = std::chrono::high_resolution_clock::now().time_since_epoch().count() - draw_time;
		m_execution_avg = 0.95 * m_execution_avg + 0.05 * draw_time / 1000000.0;
		painter.setPen(QPen(m_controls->m_colorLabels, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
		painter.drawText(displayRight -100, 10, 100, 16, Qt::AlignLeft,
						 QString("Max FPS: ").append(std::to_string(1000.0 / m_execution_avg).c_str()));
		painter.drawText(displayRight -100, 20, 100, 16, Qt::AlignLeft,
						 QString("Exec avg.: ").append(std::to_string(m_execution_avg).substr(0, 5).c_str()).append(" ms"));
	#endif
}


// Periodically trigger repaint and check if the widget is visible.
void VectorView::periodicUpdate()
{
	m_visible = isVisible();
	if (m_visible) {update();}
}

