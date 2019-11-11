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

#include <algorithm>
#include <cmath>
#include <chrono>
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
	m_displaySize(displaySize),
	m_persistTimestamp(0),
	m_oldHQ(m_controls->m_highQualityModel.value()),
	m_oldX(m_displaySize / 2),
	m_oldY(m_displaySize / 2)
{
	setMinimumSize(200, 200);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	connect(gui->mainWindow(), SIGNAL(periodicUpdate()), this, SLOT(periodicUpdate()));

	m_displayBuffer.resize(sizeof qRgb(0,0,0) * m_displaySize * m_displaySize, 0);

	#ifdef SA_DEBUG
		m_executionAvg = 0;
	#endif
}


// Compose and draw all the content; called by Qt.
void VectorView::paintEvent(QPaintEvent *event)
{
	#ifdef VEC_DEBUG
		unsigned int drawTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	#endif

	// All drawing done in this method, local variables are sufficient for the boundary
	const int displayTop = 0;
	const int displayBottom = height() -1;
	const int displayLeft = 0;
	const int displayRight = width() -1;
	const int displayWidth = displayRight - displayLeft;
	const int displayHeight = displayBottom - displayTop;

	const int centerX = displayLeft + (displayWidth / 2) + 1;
	const int centerY = displayTop + (displayWidth / 2) + 1;

	float labelWidth = 32;
	float labelHeight = 32;
	float margin = 4;

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, true);

	QFont normalFont, boldFont;
	boldFont.setPixelSize(32);
	boldFont.setBold(true);

	// Clear display buffer if quality setting was changed
	bool hq = m_controls->m_highQualityModel.value();
	if (hq != m_oldHQ)
	{
		m_oldHQ = hq;
		for (std::size_t i = 0; i < m_displayBuffer.size(); i++)
		{
			m_displayBuffer.data()[i] = 0;
		}
	}

	// Dim stored image based on persistence and elapsed time.
	// In non-HQ mode, dimming pass is limited to once every 100 ms.
	unsigned int currentTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	const unsigned int threshold = hq ? 10 : 100;
	if (currentTimestamp - m_persistTimestamp > threshold)
	{
		m_persistTimestamp = currentTimestamp;
		const std::size_t useableBuffer = hq ? m_displayBuffer.size() : m_displayBuffer.size() / 4;
		const float persist = pow(log10(1 + 9 * m_controls->m_persistenceModel.value()), threshold / 10);
		for (std::size_t i = 0; i < useableBuffer; i++)
		{
			m_displayBuffer.data()[i] = m_displayBuffer.data()[i] * persist;
		}
	}

	// Get new data from lockless FIFO buffer
	auto inBuffer = m_bufferReader.read_max(m_inputBuffer->capacity());
	std::size_t frameCount = inBuffer.size();

	int x;
	int y;
	// Draw new points on top
	if (hq)
	{
		// High quality mode: check distance between points and draw a line.
		// The longer the line is, the dimmer, simulating real electron trace on luminescent screen.
		auto saturate = [=](unsigned short pixel) {return fmax(fmin(pixel, m_displaySize - 1), 0);};
		for (std::size_t frame = 0; frame < frameCount; frame++)
		{
			float left  = inBuffer[frame][0] * (m_displaySize -1) / 4;
			float right = inBuffer[frame][1] * (m_displaySize -1) / 4;

			x = saturate(right - left + m_displaySize / 2);
			y = saturate(m_displaySize - (right + left + m_displaySize / 2));

			unsigned char points = fmin(sqrt(pow(m_oldX - x, 2.0) + pow(m_oldY - y, 2.0)), 100);
			QColor added_color = m_controls->m_colorFG.darker(100 + 10 * points).rgb();

			// Draw the new point
			QColor current_color = ((QRgb*)m_displayBuffer.data())[x + y * m_displaySize];
			current_color.setRed(fmin(current_color.red() + added_color.red(), 255));
			current_color.setGreen(fmin(current_color.green() + added_color.green(), 255));
			current_color.setBlue(fmin(current_color.blue() + added_color.blue(), 255));
			((QRgb*)m_displayBuffer.data())[x + y * m_displaySize] = current_color.rgb();

			// Draw interpolated points between the old one and the new one
			for (unsigned char i = 1; i < points; i++)
			{
				x = saturate(((points - i) * m_oldX + i * (right - left + m_displaySize / 2)) / points);
				y = saturate(((points - i) * m_oldY + i * (m_displaySize - (right + left + m_displaySize / 2))) / points);
				QColor current_color = ((QRgb*)m_displayBuffer.data())[x + y * m_displaySize];
				current_color.setRed(fmin(current_color.red() + added_color.red(), 255));
				current_color.setGreen(fmin(current_color.green() + added_color.green(), 255));
				current_color.setBlue(fmin(current_color.blue() + added_color.blue(), 255));
				((QRgb*)m_displayBuffer.data())[x + y * m_displaySize] = current_color.rgb();
			}
			m_oldX = x;
			m_oldY = y;
		}
	}
	else
	{
		// To improve performance, non-HQ mode uses smaller buffer / display size.
		const unsigned short activeSize = m_displaySize / 2;
		auto saturate = [=](unsigned short pixel) {return fmax(fmin(pixel, activeSize - 1), 0);};
		for (std::size_t frame = 0; frame < frameCount; frame++)
		{
			float left  = inBuffer[frame][0] * (activeSize -1) / 4;
			float right = inBuffer[frame][1] * (activeSize -1) / 4;
			x = saturate(right - left + activeSize / 2);
			y = saturate(activeSize - (right + left + activeSize / 2));
			((QRgb*)m_displayBuffer.data())[x + y * activeSize] = m_controls->m_colorFG.rgb();
		}
	}

	// Draw background
	painter.fillRect(displayLeft, displayTop, displayWidth, displayHeight, QColor(0,0,0));

	// Draw the final image.
	QImage temp = QImage(m_displayBuffer.data(),
						 hq ? m_displaySize : m_displaySize / 2,
						 hq ? m_displaySize : m_displaySize / 2,
						 QImage::Format_RGB32);
	temp.setDevicePixelRatio(devicePixelRatio());
	painter.drawImage(displayLeft, displayTop,
					  temp.scaledToWidth(displayWidth * devicePixelRatio(), Qt::SmoothTransformation));

	// Draw the grid and labels.
	painter.setPen(QPen(m_controls->m_colorGrid, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	painter.drawEllipse(QPoint(centerX, centerY), displayWidth / 2, displayWidth / 2);
	painter.drawLine(QPoint(centerX, centerY), QPoint(displayLeft + 50, displayTop + 50));
	painter.drawLine(QPoint(centerX, centerY), QPoint(displayRight - 50, displayTop + 50));

	painter.setFont(boldFont);
	painter.drawText(displayLeft + margin, displayTop + margin,
					 labelWidth, labelHeight, Qt::AlignLeft | Qt::AlignTop | Qt::TextDontClip,
					 QString("L"));
	painter.drawText(displayRight - margin - labelWidth, displayTop + margin,
					 labelWidth, labelHeight, Qt::AlignRight| Qt::AlignTop | Qt::TextDontClip,
					 QString("R"));
	// Draw the outline.
//	painter.setPen(QPen(m_controls->m_colorGrid, 2, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
//	painter.drawRoundedRect(displayLeft, displayTop, displayWidth, displayBottom, 2.0, 2.0);

	#ifdef VEC_DEBUG
		drawTime = std::chrono::high_resolution_clock::now().time_since_epoch().count() - drawTime;
		m_executionAvg = 0.95 * m_executionAvg + 0.05 * drawTime / 1000000.0;
		painter.setPen(QPen(m_controls->m_colorLabels, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
		painter.setFont(normalFont);
		painter.drawText(displayWidth / 2 - 50, margin, 100, 16, Qt::AlignLeft,
						 QString("Exec avg.: ").append(std::to_string(m_executionAvg).substr(0, 5).c_str()).append(" ms"));
	#endif
}


// Periodically trigger repaint and check if the widget is visible.
void VectorView::periodicUpdate()
{
	m_visible = isVisible();
	if (m_visible) {update();}
}

