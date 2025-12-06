/* VectorView.cpp - implementation of VectorView class.
 *
 * Copyright (c) 2019 Martin Pavelek <he29/dot/HS/at/gmail/dot/com>
 * Copyright (c) 2025- Michael Gregorius
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

#include <QPainter>

#include "ColorChooser.h"
#include "GuiApplication.h"
#include "FontHelper.h"
#include "MainWindow.h"
#include "VecControls.h"

namespace lmms::gui
{


VectorView::VectorView(VecControls* controls, LocklessRingBuffer<SampleFrame>* inputBuffer, QWidget* parent) :
	QWidget(parent),
	m_controls(controls),
	m_inputBuffer(inputBuffer),
	m_bufferReader(*inputBuffer),
	m_zoom(1.f),
	m_zoomTimestamp(0)
{
	setMinimumSize(200, 200);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	connect(getGUI()->mainWindow(), SIGNAL(periodicUpdate()), this, SLOT(periodicUpdate()));

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

	const bool logScale = m_controls->getLogarithmicModel().value();
	const bool linesMode = m_controls->getLinesModel().value();

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, true);

	// Paint background
	painter.fillRect(rect(), Qt::black);

	const qreal widthF = qreal(width());
	const qreal heightF = qreal(height());

	const auto minOfWidthAndHeight = std::min(widthF, heightF);
	// If we would divide by 4 then the circle would go to the boundaries
	// of the widget. We increase the value at bit more to get some margin.
	const auto scaleValue = minOfWidthAndHeight / 4.1;

	// Compute several transforms that are used to paint various elements

	// This transform moves the origin/center to the middle of the widget width and to the correct height
	QTransform centerTransform;
	centerTransform.translate(widthF / 2., minOfWidthAndHeight / 2.);

	// This transform is used to center and scale the painting of data and of the grid and labels
	QTransform gridAndLabelTransform(centerTransform);
	// Invert the Y axis while we are at it so that we can paint in a "normal" coordinate system
	gridAndLabelTransform.scale(scaleValue, -scaleValue);

	// This transform is used to paint the traces. It takes the zoom factor as an "extra" scale into account as well.
	QTransform tracePaintingTransform(gridAndLabelTransform);
	tracePaintingTransform.scale(m_zoom, m_zoom);

	const auto traceWidth = 2. / (scaleValue * m_zoom);

	// This will add colors so that line intersections produce lighter colors/intensities
	painter.setCompositionMode(QPainter::CompositionMode_Plus);
	painter.setTransform(tracePaintingTransform);

	// Get new samples from the lockless input FIFO buffer
	const auto inBuffer = m_bufferReader.read_max(m_inputBuffer->capacity());
	const std::size_t frameCount = inBuffer.size();

	for (std::size_t frame = 0; frame < frameCount; ++frame)
	{
		auto sampleFrame = inBuffer[frame];

		if (logScale)
		{
			const float distance = std::sqrt(sampleFrame.sumOfSquaredAmplitudes());
			const float distanceLog = std::log10(1 + 9 * std::abs(distance));

			if (distance != 0)
			{
				const float factor = distanceLog / distance;
				sampleFrame *= factor;
			}
		}

		// Perform a mid/side split which will potentially boost signals
		//
		// Represent the side by the x coordinate and the mid by the y coordinate.
		// 
		// A mono signal which just contains a mid signal will just show as a line
		// along the y axis because it carries the same information in the left and right channel.
		// So we can say: left == right. So lets replace "right" with "left" in the formula below:
		// (left - left, -(left + left)) = (0, -2*left).
		// If two signals are completely out of phase the show as a line along the x axis. That's because
		// each signal is the opposite of the other one, e.g. right = -left. Let's replace again:
		// (left - (-left), -(left - left)) = (2*left, 0).
		//
		const auto mid = sampleFrame.left() + sampleFrame.right();
		const auto side = sampleFrame.left() - sampleFrame.right();

		// We negate the mid value of the coordinate so that it tilts correctly if we pan hard left and hard right
		QPointF currentPoint(side, -mid);

		const auto darkenedColor(m_colorTrace.darker(100 + frame));
		painter.setPen(QPen(darkenedColor, traceWidth));

		// Only draw a line if we can draw a line, i.e. if the point really changes.
		// Otherwise just produce a point.
		// Without this check Qt will draw horizontal lines when silence is processed.
		if (linesMode && m_lastPoint != currentPoint)
		{
			painter.drawLine(QLineF(m_lastPoint, currentPoint));
		}
		else
		{
			painter.drawPoint(currentPoint);
		}

		m_lastPoint = currentPoint;
	}

	// Draw grid and labels overlay
	painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
	painter.setTransform(gridAndLabelTransform);

	const QPointF origin(0, 0);
	painter.setPen(QPen(m_colorGrid, 2.5 / scaleValue, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	painter.drawEllipse(origin, 2.f, 2.f);

	const qreal root = std::sqrt(qreal(2.1));
	painter.setPen(QPen(m_colorGrid, 2.5 / scaleValue, Qt::DotLine, Qt::RoundCap, Qt::BevelJoin));
	painter.drawLine(origin, QPointF(-root, root));
	painter.drawLine(origin, QPointF(root, root));

	painter.resetTransform();

	// Draw L/R text
	const auto lText = QString("L");
	const auto rText = QString("R");

	QFont boldFont = adjustedToPixelSize(painter.font(), 26);
	boldFont.setBold(true);

	QFontMetrics fm(boldFont);
	const auto boundingRectL = fm.boundingRect(lText);
	const auto boundingRectR = fm.boundingRect(rText);

	painter.setPen(QPen(m_colorLabels, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	painter.setFont(boldFont);

	QTransform transformL(centerTransform);
	transformL.rotate(-45.);
	transformL.translate(-boundingRectL.width() / 2, -(minOfWidthAndHeight / 2) - 10);
	painter.setTransform(transformL);
	painter.drawText(0, 0, lText);

	QTransform transformR(centerTransform);
	transformR.rotate(45.);
	transformR.translate(-boundingRectR.width() / 2, -(minOfWidthAndHeight / 2) - 10);
	painter.setTransform(transformR);
	painter.drawText(0, 0, rText);

	drawZoomInfo();

		// Optionally measure drawing performance
#ifdef VEC_DEBUG
	QPainter debugPainter(this);

	drawTime = std::chrono::high_resolution_clock::now().time_since_epoch().count() - drawTime;
	m_executionAvg = 0.95f * m_executionAvg + 0.05f * drawTime / 1000000.f;

	QString debugText = tr("Exec avg.: %1 ms").arg(static_cast<int>(round(m_executionAvg)));
	debugPainter.setPen(QPen(m_controls->m_colorLabels, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	debugPainter.drawText(0, height(), debugText);
#endif
}


// Periodically trigger repaint and check if the widget is visible
void VectorView::periodicUpdate()
{
	if (isVisible())
	{
		update();
	}
}


// Allow to change color on double-click.
// More of an Easter egg, to avoid cluttering the interface with non-essential functionality.
void VectorView::mouseDoubleClickEvent(QMouseEvent *event)
{
	auto colorDialog = new ColorChooser(m_colorTrace, this);
	if (colorDialog->exec())
	{
		m_colorTrace = colorDialog->currentColor();
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

void VectorView::drawZoomInfo()
{
	const unsigned int currentTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>
	(
		std::chrono::high_resolution_clock::now().time_since_epoch()
	).count();

	if (currentTimestamp - m_zoomTimestamp < 1000)
	{
		QPainter painter(this);

		const auto zoomValue = static_cast<int>(std::round(m_zoom * 100.));
		const auto text = tr("Zoom: %1 %").arg(zoomValue);

		// Measure text
		const auto fm = painter.fontMetrics();
		const auto boundingRect = fm.boundingRect(text);
		const auto descent = fm.descent();

		painter.setPen(QPen(m_colorLabels, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
		painter.drawText((width() - boundingRect.width()) / 2, height() - descent - 2, text);
	}
}


} // namespace lmms::gui
