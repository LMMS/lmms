/* VectorView.cpp - implementation of VectorView class.
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
#include <chrono>
#include <cmath>
#include <QImage>
#include <QPainter>

#include "ColorChooser.h"
#include "GuiApplication.h"
#include "MainWindow.h"


VectorView::VectorView(VecControls *controls, LocklessRingBuffer<sampleFrame> *inputBuffer, unsigned short displaySize, QWidget *parent) :
	QWidget(parent),
	m_controls(controls),
	m_inputBuffer(inputBuffer),
	m_bufferReader(*inputBuffer),
	m_displaySize(displaySize),
	m_zoom(1.f),
	m_persistTimestamp(0),
	m_zoomTimestamp(0),
	m_oldHQ(m_controls->m_highQualityModel.value()),
	m_oldX(m_displaySize / 2),
	m_oldY(m_displaySize / 2)
{
	setMinimumSize(200, 200);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	connect(gui->mainWindow(), SIGNAL(periodicUpdate()), this, SLOT(periodicUpdate()));

	m_displayBuffer.resize(sizeof qRgb(0,0,0) * m_displaySize * m_displaySize, 0);

#ifdef VEC_DEBUG
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
	const int displayTop = 2;
	const int displayBottom = height() - 2;
	const int displayLeft = 2;
	const int displayRight = width() - 2;
	const int displayWidth = displayRight - displayLeft;
	const int displayHeight = displayBottom - displayTop;

	const float centerX = displayLeft + (displayWidth / 2.f);
	const float centerY = displayTop + (displayWidth / 2.f);

	const int margin = 4;
	const int gridCorner = 30;

	// Setup QPainter and font sizes
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, true);

	QFont normalFont, boldFont;
	boldFont.setPixelSize(26);
	boldFont.setBold(true);
	const int labelWidth = 26;
	const int labelHeight = 26;

	bool hq = m_controls->m_highQualityModel.value();

	// Clear display buffer if quality setting was changed
	if (hq != m_oldHQ)
	{
		m_oldHQ = hq;
		for (std::size_t i = 0; i < m_displayBuffer.size(); i++)
		{
			m_displayBuffer.data()[i] = 0;
		}
	}

	// Dim stored image based on persistence setting and elapsed time.
	// Update period is limited to 50 ms (20 FPS) for non-HQ mode and 10 ms (100 FPS) for HQ mode.
	const unsigned int currentTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>
	(
		std::chrono::high_resolution_clock::now().time_since_epoch()
	).count();
	const unsigned int elapsed = currentTimestamp - m_persistTimestamp;
	const unsigned int threshold = hq ? 10 : 50;
	if (elapsed > threshold)
	{
		m_persistTimestamp = currentTimestamp;
		// Non-HQ mode uses half the resolution → use limited buffer space.
		const std::size_t useableBuffer = hq ? m_displayBuffer.size() : m_displayBuffer.size() / 4;
		// The knob value is interpreted on log. scale, otherwise the effect would ramp up too slowly.
		// Persistence value specifies fraction of light intensity that remains after 10 ms.
		// → Compensate it based on elapsed time (exponential decay).
		const float persist = log10(1 + 9 * m_controls->m_persistenceModel.value());
		const float persistPerFrame = pow(persist, elapsed / 10.f);
		// Note that for simplicity and performance reasons, this implementation only dims all stored
		// values by a given factor. A true simulation would also do the inverse of desaturation that
		// occurs in high-intensity traces in HQ mode.
		for (std::size_t i = 0; i < useableBuffer; i++)
		{
			m_displayBuffer.data()[i] *= persistPerFrame;
		}
	}

	// Get new samples from the lockless input FIFO buffer
	auto inBuffer = m_bufferReader.read_max(m_inputBuffer->capacity());
	std::size_t frameCount = inBuffer.size();

	// Draw new points on top
	float left, right;
	int x, y;

	const bool logScale = m_controls->m_logarithmicModel.value();
	const unsigned short activeSize = hq ? m_displaySize : m_displaySize / 2;

	// Helper lambda functions for better readability
	// Make sure pixel stays within display bounds:
	auto saturate = [=](short pixelPos) {return qBound((short)0, pixelPos, (short)(activeSize - 1));};
	// Take existing pixel and brigthen it. Very bright light should reduce saturation and become
	// white. This effect is easily approximated by capping elementary colors to 255 individually.
	auto updatePixel = [&](unsigned short x, unsigned short y, QColor addedColor)
	{
		QColor currentColor = ((QRgb*)m_displayBuffer.data())[x + y * activeSize];
		currentColor.setRed(std::min(currentColor.red() + addedColor.red(), 255));
		currentColor.setGreen(std::min(currentColor.green() + addedColor.green(), 255));
		currentColor.setBlue(std::min(currentColor.blue() + addedColor.blue(), 255));
		((QRgb*)m_displayBuffer.data())[x + y * activeSize] = currentColor.rgb();
	};

	if (hq)
	{
		// High quality mode: check distance between points and draw a line.
		// The longer the line is, the dimmer, simulating real electron trace on luminescent screen.
		for (std::size_t frame = 0; frame < frameCount; frame++)
		{
			float inLeft = inBuffer[frame][0] * m_zoom;
			float inRight = inBuffer[frame][1] * m_zoom;
			// Scale left and right channel from (-1.0, 1.0) to display range
			if (logScale)
			{
				// To better preserve shapes, the log scale is applied to the distance from origin,
				// not the individual channels.
				const float distance = sqrt(inLeft * inLeft + inRight * inRight);
				const float distanceLog = log10(1 + 9 * abs(distance));
				const float angleCos = inLeft / distance;
				const float angleSin = inRight / distance;
				left  = distanceLog * angleCos * (activeSize - 1) / 4;
				right = distanceLog * angleSin * (activeSize - 1) / 4;
			}
			else
			{
				left  = inLeft * (activeSize - 1) / 4;
				right = inRight * (activeSize - 1) / 4;
			}

			// Rotate display coordinates 45 degrees, flip Y axis and make sure the result stays within bounds
			x = saturate(right - left + activeSize / 2.f);
			y = saturate(activeSize - (right + left + activeSize / 2.f));

			// Estimate number of points needed to fill space between the old and new pixel. Cap at 100.
			unsigned char points = std::min((int)sqrt((m_oldX - x) * (m_oldX - x) + (m_oldY - y) * (m_oldY - y)), 100);

			// Large distance = dim trace. The curve for darker() is choosen so that:
			// - no movement (0 points) actually _increases_ brightness slightly,
			// - one point between samples = returns exactly the specified color,
			// - one to 99 points between samples = follows a sharp "1/x" decaying curve,
			// - 100 points between samples = returns approximately 5 % brightness.
			// Everything else is discarded (by the 100 point cap) because there is not much to see anyway.
			QColor addedColor = m_controls->m_colorFG.darker(75 + 20 * points).rgb();

			// Draw the new pixel: the beam sweeps across area that may have been excited before
			// → add new value to existing pixel state.
			updatePixel(x, y, addedColor);

			// Draw interpolated points between the old pixel and the new one
			int newX = right - left + activeSize / 2.f;
			int newY = activeSize - (right + left + activeSize / 2.f);
			for (unsigned char i = 1; i < points; i++)
			{
				x = saturate(((points - i) * m_oldX + i * newX) / points);
				y = saturate(((points - i) * m_oldY + i * newY) / points);
				updatePixel(x, y, addedColor);
			}
			m_oldX = newX;
			m_oldY = newY;
		}
	}
	else
	{
		// To improve performance, non-HQ mode uses smaller display size and only
		// one full-color pixel per sample.
		for (std::size_t frame = 0; frame < frameCount; frame++)
		{
			float inLeft = inBuffer[frame][0] * m_zoom;
			float inRight = inBuffer[frame][1] * m_zoom;
			if (logScale) {
				const float distance = sqrt(inLeft * inLeft + inRight * inRight);
				const float distanceLog = log10(1 + 9 * abs(distance));
				const float angleCos = inLeft / distance;
				const float angleSin = inRight / distance;
				left  = distanceLog * angleCos * (activeSize - 1) / 4;
				right = distanceLog * angleSin * (activeSize - 1) / 4;
			} else {
				left  = inLeft * (activeSize - 1) / 4;
				right = inRight * (activeSize - 1) / 4;
			}
			x = saturate(right - left + activeSize / 2.f);
			y = saturate(activeSize - (right + left + activeSize / 2.f));
			((QRgb*)m_displayBuffer.data())[x + y * activeSize] = m_controls->m_colorFG.rgb();
		}
	}

	// Draw background
	painter.fillRect(displayLeft, displayTop, displayWidth, displayHeight, QColor(0,0,0));

	// Draw the final image
	QImage temp = QImage(m_displayBuffer.data(),
						 activeSize,
						 activeSize,
						 QImage::Format_RGB32);
	temp.setDevicePixelRatio(devicePixelRatio());
	painter.drawImage(displayLeft, displayTop,
					  temp.scaledToWidth(displayWidth * devicePixelRatio(),
					  Qt::SmoothTransformation));

	// Draw the grid and labels
	painter.setPen(QPen(m_controls->m_colorGrid, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	painter.drawEllipse(QPointF(centerX, centerY), displayWidth / 2.f, displayWidth / 2.f);
	painter.setPen(QPen(m_controls->m_colorGrid, 1.5, Qt::DotLine, Qt::RoundCap, Qt::BevelJoin));
	painter.drawLine(QPointF(centerX, centerY), QPointF(displayLeft + gridCorner, displayTop + gridCorner));
	painter.drawLine(QPointF(centerX, centerY), QPointF(displayRight - gridCorner, displayTop + gridCorner));

	painter.setPen(QPen(m_controls->m_colorLabels, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	painter.setFont(boldFont);
	painter.drawText(displayLeft + margin, displayTop,
					 labelWidth, labelHeight, Qt::AlignLeft | Qt::AlignTop | Qt::TextDontClip,
					 QString("L"));
	painter.drawText(displayRight - margin - labelWidth, displayTop,
					 labelWidth, labelHeight, Qt::AlignRight| Qt::AlignTop | Qt::TextDontClip,
					 QString("R"));

	// Draw the outline
	painter.setPen(QPen(m_controls->m_colorOutline, 2, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	painter.drawRoundedRect(1, 1, width() - 2, height() - 2, 2.f, 2.f);

	// Draw zoom info if changed within last second (re-using timestamp acquired for dimming)
	if (currentTimestamp - m_zoomTimestamp < 1000)
	{
		painter.setPen(QPen(m_controls->m_colorLabels, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
		painter.setFont(normalFont);
		painter.drawText(displayWidth / 2 - 50, displayBottom - 20, 100, 16, Qt::AlignCenter,
						 QString("Zoom: ").append(std::to_string((int)round(m_zoom * 100)).c_str()).append(" %"));
	}

	// Optionally measure drawing performance
#ifdef VEC_DEBUG
	drawTime = std::chrono::high_resolution_clock::now().time_since_epoch().count() - drawTime;
	m_executionAvg = 0.95f * m_executionAvg + 0.05f * drawTime / 1000000.f;
	painter.setPen(QPen(m_controls->m_colorLabels, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	painter.setFont(normalFont);
	painter.drawText(displayWidth / 2 - 50, displayBottom - 16, 100, 16, Qt::AlignLeft,
					 QString("Exec avg.: ").append(std::to_string(m_executionAvg).substr(0, 5).c_str()).append(" ms"));
#endif
}


// Periodically trigger repaint and check if the widget is visible
void VectorView::periodicUpdate()
{
	m_visible = isVisible();
	if (m_visible) {update();}
}


// Allow to change color on double-click.
// More of an Easter egg, to avoid cluttering the interface with non-essential functionality.
void VectorView::mouseDoubleClickEvent(QMouseEvent *event)
{
	ColorChooser *colorDialog = new ColorChooser(m_controls->m_colorFG, this);
	if (colorDialog->exec())
	{
		m_controls->m_colorFG = colorDialog->currentColor();
	}
}


// Change zoom level using the mouse wheel
void VectorView::wheelEvent(QWheelEvent *event)
{
	// Go through integers to avoid accumulating errors
	const unsigned short old_zoom = round(100 * m_zoom);
	// Min-max bounds are 20 and 1000 %, step for 15°-increment mouse wheel is 20 %
	const unsigned short new_zoom = qBound(20, old_zoom + event->angleDelta().y() / 6, 1000);
	m_zoom = new_zoom / 100.f;
	event->accept();
	m_zoomTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>
	(
		std::chrono::high_resolution_clock::now().time_since_epoch()
	).count();

}
