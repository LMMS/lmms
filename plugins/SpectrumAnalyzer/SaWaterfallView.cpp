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
#include <cmath>
#include <QImage>
#include <QMutexLocker>
#include <QPainter>
#include <QSplitter>
#include <QString>

#include "EffectControlDialog.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "SaProcessor.h"


SaWaterfallView::SaWaterfallView(SaControls *controls, SaProcessor *processor, QWidget *_parent) :
	QWidget(_parent),
	m_controls(controls),
	m_processor(processor)
{
	m_controlDialog = (EffectControlDialog*) _parent;
	setMinimumSize(300, 150);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	connect(gui->mainWindow(), SIGNAL(periodicUpdate()), this, SLOT(periodicUpdate()));

	m_timeTics = makeTimeTics();
	m_oldTimePerLine = (float)m_processor->m_inBlockSize / m_processor->getSampleRate();
}


// Compose and draw all the content; called by Qt.
// Not as performance sensitive as SaSpectrumView, most of the processing is
// done directly in SaProcessor.
void SaWaterfallView::paintEvent(QPaintEvent *event)
{
	#ifdef SA_DEBUG
		int start_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	#endif

	// all drawing done here, local variables are sufficient for the boundary
	const int displayTop = 1;
	const int displayBottom = height() -2;
	const int displayLeft = 26;
	const int displayRight = width() -26;
	const int displayWidth = displayRight - displayLeft;
	float label_width = 20;
	float label_height = 16;
	float margin = 2;

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, true);

	// check if time labels need to be rebuilt
	if ((float)m_processor->m_inBlockSize / m_processor->getSampleRate() != m_oldTimePerLine)
	{
		m_timeTics = makeTimeTics();
		m_oldTimePerLine = (float)m_processor->m_inBlockSize / m_processor->getSampleRate();
	}

	// print time labels
	float pos = 0;
	painter.setPen(QPen(m_controls->m_colorLabels, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	for (auto & line: m_timeTics)
	{
		pos = timeToYPixel(line.first, displayBottom);
		// align first and last label to the edge if needed, otherwise center them
		if (line == m_timeTics.front() && pos < label_height / 2)
		{
			painter.drawText(displayLeft - label_width - margin, displayTop - 1,
							 label_width, label_height, Qt::AlignRight | Qt::AlignTop | Qt::TextDontClip,
							 QString(line.second.c_str()));
			painter.drawText(displayRight + margin, displayTop - 1,
							 label_width, label_height, Qt::AlignLeft | Qt::AlignTop | Qt::TextDontClip,
							 QString(line.second.c_str()));
		}
		else if (line == m_timeTics.back() && pos > displayBottom - label_height + 2)
		{
			painter.drawText(displayLeft - label_width - margin, displayBottom - label_height,
							 label_width, label_height, Qt::AlignRight | Qt::AlignBottom | Qt::TextDontClip,
							 QString(line.second.c_str()));
			painter.drawText(displayRight + margin, displayBottom - label_height + 2,
							 label_width, label_height, Qt::AlignLeft | Qt::AlignBottom | Qt::TextDontClip,
							 QString(line.second.c_str()));
		}
		else
		{
			painter.drawText(displayLeft - label_width - margin, pos - label_height / 2,
							 label_width, label_height, Qt::AlignRight | Qt::AlignVCenter | Qt::TextDontClip,
							 QString(line.second.c_str()));
			painter.drawText(displayRight + margin, pos - label_height / 2,
							 label_width, label_height, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextDontClip,
							 QString(line.second.c_str()));
		}
	}

	// draw the spectrogram precomputed in SaProcessor
	if (m_processor->m_waterfallNotEmpty)
	{
		QMutexLocker lock(&m_processor->m_dataAccess);
		painter.drawImage(displayLeft, displayTop,					// top left corner coordinates
						  QImage(m_processor->m_history.data(),		// raw pixel data to display
								 m_processor->binCount(),			// width = number of frequency bins
								 m_processor->m_waterfallHeight,	// height = number of history lines
								 QImage::Format_RGB32
								 ).scaled(displayWidth,				// scale to fit view..
										  displayBottom,
										  Qt::IgnoreAspectRatio,
										  Qt::SmoothTransformation));
		lock.unlock();
	}
	else
	{
		painter.fillRect(displayLeft, displayTop, displayWidth, displayBottom, QColor(0,0,0));
	}

	// always draw the outline
	painter.setPen(QPen(m_controls->m_colorGrid, 2, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	painter.drawRoundedRect(displayLeft, displayTop, displayWidth, displayBottom, 2.0, 2.0);

	#ifdef SA_DEBUG
		// display what FPS would be achieved if waterfall ran in a loop
		start_time = std::chrono::high_resolution_clock::now().time_since_epoch().count() - start_time;
		painter.setPen(QPen(m_controls->m_colorLabels, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
		painter.drawText(displayRight -100, 10, 100, 16, Qt::AlignLeft,
						 QString(std::string("Max FPS: " + std::to_string(1000000000.0 / start_time)).c_str()));
	#endif
}


// Convert time value to Y coordinate for display of given height.
float SaWaterfallView::timeToYPixel(float time, int height)
{
	float pixels_per_line = (float)height / m_processor->m_waterfallHeight;
	float seconds_per_line = ((float)m_processor->m_inBlockSize / m_processor->getSampleRate());

	return pixels_per_line * time / seconds_per_line;
}


// Generate labels for linear time scale.
std::vector<std::pair<float, std::string>> SaWaterfallView::makeTimeTics()
{
	std::vector<std::pair<float, std::string>> result;
	float i;

	// upper limit defined by number of lines * time per line
	float limit = m_processor->m_waterfallHeight * ((float)m_processor->m_inBlockSize / m_processor->getSampleRate());

	// set increment so that about 8 tics are generated
	float increment = std::round(10 * limit / 7) / 10;

	// NOTE: labels positions are rounded to match the (rounded) label value
	for (i = 0; i <= limit; i += increment)
	{
		if (i < 10)
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
		QMutexLocker lock(&m_processor->m_dataAccess);
		std::fill(m_processor->m_history.begin(), m_processor->m_history.end(), 0);
		lock.unlock();

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

