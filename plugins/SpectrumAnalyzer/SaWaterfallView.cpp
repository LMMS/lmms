/* SaWaterfallViewView.cpp - implementation of SaWaterfallViewView class.
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

#include "SaWaterfallView.h"

#include <algorithm>
#ifdef SA_DEBUG
	#include <chrono>
#endif
#include <cmath>
#include <QImage>
#include <QMouseEvent>
#include <QMutexLocker>
#include <QPainter>
#include <QString>

#include "DeprecationHelper.h"
#include "EffectControlDialog.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "SaControls.h"
#include "SaProcessor.h"


namespace lmms::gui
{


SaWaterfallView::SaWaterfallView(SaControls *controls, SaProcessor *processor, QWidget *_parent) :
	QWidget(_parent),
	m_controls(controls),
	m_processor(processor)
{
	m_controlDialog = (EffectControlDialog*) _parent;
	setMinimumSize(300, 150);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	connect(getGUI()->mainWindow(), SIGNAL(periodicUpdate()), this, SLOT(periodicUpdate()));
	connect(&controls->m_waterfallModel, &BoolModel::dataChanged, this, &SaWaterfallView::updateVisibility);

	m_displayTop = 1;
	m_displayBottom = height() -2;
	m_displayLeft = 26;
	m_displayRight = width() -26;
	m_displayWidth = m_displayRight - m_displayLeft;
	m_displayHeight = m_displayBottom - m_displayTop;

	m_timeTics = makeTimeTics();
	m_oldSecondsPerLine = 0;
	m_oldHeight = 0;

	m_cursor = QPointF(0, 0);

	#ifdef SA_DEBUG
		m_execution_avg = 0;
	#endif
}


// Compose and draw all the content; called by Qt.
// Not as performance sensitive as SaSpectrumView, most of the processing is
// done directly in SaProcessor.
void SaWaterfallView::paintEvent(QPaintEvent *event)
{
	#ifdef SA_DEBUG
		unsigned int draw_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	#endif

	// update boundary
	m_displayBottom = height() -2;
	m_displayRight = width() -26;
	m_displayWidth = m_displayRight - m_displayLeft;
	m_displayHeight = m_displayBottom - m_displayTop;
	float label_width = 20;
	float label_height = 16;
	float margin = 2;

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, true);

	// check if time labels need to be rebuilt
	if (secondsPerLine() != m_oldSecondsPerLine || m_processor->waterfallHeight() != m_oldHeight)
	{
		m_timeTics = makeTimeTics();
		m_oldSecondsPerLine = secondsPerLine();
		m_oldHeight = m_processor->waterfallHeight();
	}

	// print time labels
	float pos = 0;
	painter.setPen(QPen(m_controls->m_colorLabels, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	for (auto & line: m_timeTics)
	{
		pos = timeToYPixel(line.first, m_displayHeight);
		// align first and last label to the edge if needed, otherwise center them
		if (line == m_timeTics.front() && pos < label_height / 2)
		{
			painter.drawText(m_displayLeft - label_width - margin, m_displayTop - 1,
							 label_width, label_height, Qt::AlignRight | Qt::AlignTop | Qt::TextDontClip,
							 QString(line.second.c_str()));
			painter.drawText(m_displayRight + margin, m_displayTop - 1,
							 label_width, label_height, Qt::AlignLeft | Qt::AlignTop | Qt::TextDontClip,
							 QString(line.second.c_str()));
		}
		else if (line == m_timeTics.back() && pos > m_displayBottom - label_height + 2)
		{
			painter.drawText(m_displayLeft - label_width - margin, m_displayBottom - label_height,
							 label_width, label_height, Qt::AlignRight | Qt::AlignBottom | Qt::TextDontClip,
							 QString(line.second.c_str()));
			painter.drawText(m_displayRight + margin, m_displayBottom - label_height + 2,
							 label_width, label_height, Qt::AlignLeft | Qt::AlignBottom | Qt::TextDontClip,
							 QString(line.second.c_str()));
		}
		else
		{
			painter.drawText(m_displayLeft - label_width - margin, pos - label_height / 2,
							 label_width, label_height, Qt::AlignRight | Qt::AlignVCenter | Qt::TextDontClip,
							 QString(line.second.c_str()));
			painter.drawText(m_displayRight + margin, pos - label_height / 2,
							 label_width, label_height, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextDontClip,
							 QString(line.second.c_str()));
		}
	}

	// draw the spectrogram precomputed in SaProcessor
	if (m_processor->waterfallNotEmpty())
	{
		QMutexLocker lock(&m_processor->m_reallocationAccess);
		QImage temp = QImage(m_processor->getHistory(),			// raw pixel data to display
							 m_processor->waterfallWidth(),		// width = number of frequency bins
							 m_processor->waterfallHeight(),	// height = number of history lines
							 QImage::Format_RGB32);
		lock.unlock();
		temp.setDevicePixelRatio(devicePixelRatio());			// display at native resolution
		painter.drawImage(m_displayLeft, m_displayTop,
						  temp.scaled(m_displayWidth * devicePixelRatio(),
									  m_displayHeight * devicePixelRatio(),
									  Qt::IgnoreAspectRatio,
									  Qt::SmoothTransformation));
		m_processor->flipRequest();
	}
	else
	{
		painter.fillRect(m_displayLeft, m_displayTop, m_displayWidth, m_displayHeight, QColor(0,0,0));
	}

	// draw cursor (if it is within bounds)
	drawCursor(painter);

	// always draw the outline
	painter.setPen(QPen(m_controls->m_colorGrid, 2, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	painter.drawRoundedRect(m_displayLeft, m_displayTop, m_displayWidth, m_displayHeight, 2.0, 2.0);

	#ifdef SA_DEBUG
		draw_time = std::chrono::high_resolution_clock::now().time_since_epoch().count() - draw_time;
		m_execution_avg = 0.95 * m_execution_avg + 0.05 * draw_time / 1000000.0;
		painter.setPen(QPen(m_controls->m_colorLabels, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
		painter.drawText(m_displayRight -150, 10, 100, 16, Qt::AlignLeft,
						 QString("Exec avg.: ").append(std::to_string(m_execution_avg).substr(0, 5).c_str()).append(" ms"));
	#endif
}


// Helper functions for time conversion
float SaWaterfallView::samplesPerLine()
{
	return (float)m_processor->inBlockSize() / m_controls->m_windowOverlapModel.value();
}

float SaWaterfallView::secondsPerLine()
{
	return samplesPerLine() / m_processor->getSampleRate();
}


// Convert time value to Y coordinate for display of given height.
float SaWaterfallView::timeToYPixel(float time, int height)
{
	float pixels_per_line = (float)height / m_processor->waterfallHeight();

	return pixels_per_line * time / secondsPerLine();
}


// Convert Y coordinate on display of given height back to time value.
float SaWaterfallView::yPixelToTime(float position, int height)
{
	if (height == 0) {height = 1;}
	float pixels_per_line = (float)height / m_processor->waterfallHeight();

	return (position / pixels_per_line) * secondsPerLine();
}


// Generate labels for linear time scale.
std::vector<std::pair<float, std::string>> SaWaterfallView::makeTimeTics()
{
	std::vector<std::pair<float, std::string>> result;

	// get time value of the last line
	float limit = yPixelToTime(m_displayBottom, m_displayHeight);

	// set increment to about 30 pixels (but min. 0.1 s)
	const float increment = std::max(std::round(10 * limit / (m_displayHeight / 30)) / 10, 0.1f);

	// NOTE: labels positions are rounded to match the (rounded) label value
	for (float i = 0; i <= limit; i += increment)
	{
		if (i > 99)
		{
			result.emplace_back(std::round(i), std::to_string(std::round(i)).substr(0, 3));
		}
		else if (i < 10)
		{
			result.emplace_back(std::round(i * 10) / 10, std::to_string(std::round(i * 10) / 10).substr(0, 3));
		}
		else
		{
			result.emplace_back(std::round(i), std::to_string(std::round(i)).substr(0, 2));
		}
	}
	return result;
}


// Periodically trigger repaint and check if the widget is visible.
// If it is not, stop drawing and inform the processor.
void SaWaterfallView::periodicUpdate()
{
	m_processor->setWaterfallActive(isVisible());
	if (isVisible()) {update();}
}


// Adjust window size and widget visibility when waterfall is enabled or disabbled.
void SaWaterfallView::updateVisibility()
{
	// get container of the control dialog to be resized if needed
	QWidget *subWindow = m_controlDialog->parentWidget();


	if (m_controls->m_waterfallModel.value())
	{
		// clear old data before showing the waterfall
		m_processor->clearHistory();
		setVisible(true);

		// increase window size if it is too small
		if (subWindow->size().height() < m_controlDialog->sizeHint().height())
		{
			subWindow->resize(subWindow->size().width(), m_controlDialog->sizeHint().height());
		}
	}
	else
	{
		setVisible(false);
		// decrease window size only if it does not violate sizeHint
		subWindow->resize(subWindow->size().width(), m_controlDialog->sizeHint().height());
	}
}


// Draw cursor and its coordinates if it is within display bounds.
void SaWaterfallView::drawCursor(QPainter &painter)
{
	if (	m_cursor.x() >= m_displayLeft
		&&	m_cursor.x() <= m_displayRight
		&&	m_cursor.y() >= m_displayTop
		&&	m_cursor.y() <= m_displayBottom)
	{
		// cursor lines
		painter.setPen(QPen(m_controls->m_colorGrid.lighter(), 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
		painter.drawLine(QPointF(m_cursor.x(), m_displayTop), QPointF(m_cursor.x(), m_displayBottom));
		painter.drawLine(QPointF(m_displayLeft, m_cursor.y()), QPointF(m_displayRight, m_cursor.y()));

		// coordinates: background box
		QFontMetrics fontMetrics = painter.fontMetrics();
		unsigned int const box_left = 5;
		unsigned int const box_top = 5;
		unsigned int const box_margin = 3;
		unsigned int const box_height = 2*(fontMetrics.size(Qt::TextSingleLine, "0 Hz").height() + box_margin);
		unsigned int const box_width = fontMetrics.size(Qt::TextSingleLine, "20000 Hz ").width() + 2*box_margin;
		painter.setPen(QPen(m_controls->m_colorLabels.darker(), 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
		painter.fillRect(m_displayLeft + box_left, m_displayTop + box_top,
						 box_width, box_height, QColor(0, 0, 0, 64));

		// coordinates: text
		painter.setPen(QPen(m_controls->m_colorLabels, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
		QString tmps;

		// frequency
		int freq = (int)m_processor->xPixelToFreq(m_cursor.x() - m_displayLeft, m_displayWidth);
		tmps = QString("%1 Hz").arg(freq);
		painter.drawText(m_displayLeft + box_left + box_margin,
						 m_displayTop + box_top + box_margin,
						 box_width, box_height / 2, Qt::AlignLeft, tmps);

		// time
		float time = yPixelToTime(m_cursor.y(), m_displayBottom);
		tmps = QString(std::to_string(time).substr(0, 5).c_str()).append(" s");
		painter.drawText(m_displayLeft + box_left + box_margin,
						 m_displayTop + box_top + box_height / 2,
						 box_width, box_height / 2, Qt::AlignLeft, tmps);
	}
}


// Handle mouse input: set new cursor position.
void SaWaterfallView::mouseMoveEvent(QMouseEvent* event)
{
	m_cursor = positionF(event);
}

void SaWaterfallView::mousePressEvent(QMouseEvent* event)
{
	m_cursor = positionF(event);
}


// Handle resize event: rebuild time labels
void SaWaterfallView::resizeEvent(QResizeEvent *event)
{
	m_timeTics = makeTimeTics();
}


} // namespace lmms::gui
