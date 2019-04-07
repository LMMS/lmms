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

#include <mutex>
#include <QSplitter>
#include <QWidget>

#include "EffectControlDialog.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "SaProcessor.h"


SaWaterfallView::SaWaterfallView(SaControls *controls, SaProcessor *processor, QWidget *_parent) :
	QWidget(_parent),
	m_controls(controls),
	m_processor(processor)
{
	setMinimumSize(300, 150);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	connect(gui->mainWindow(), SIGNAL(periodicUpdate()), this, SLOT(periodicUpdate()));

	m_timeTics = makeTimeTics();
	m_oldTimePerLine = (float)m_processor->m_inBlockSize / m_processor->getSampleRate();
}


// Compose and draw all the content; periodically called by Qt.
// Not as performance sensitive as SaSpectrumView, most of the processing is
// done directly in SaProcessor.
void SaWaterfallView::paintEvent(QPaintEvent *event) {

	#ifdef SA_DEBUG
		int start_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	#endif

	const int displayBottom = height();
	const int displayLeft = 20;
	const int displayRight = width() -20;
	const int displayWidth = displayRight - displayLeft;
	float pos = 0;

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, true);

	// check if time labels need to be rebuilt
	if ((float)m_processor->m_inBlockSize / m_processor->getSampleRate() != m_oldTimePerLine) {
		m_timeTics = makeTimeTics();
		m_oldTimePerLine = (float)m_processor->m_inBlockSize / m_processor->getSampleRate();
	}

	painter.setPen(QPen(m_controls->m_colorLabels, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	for (auto & line: m_timeTics) {
		pos = timeToYPixel(line.first, displayBottom);
		// make an exception if the top or bottom label is too close to edge
		if (line == m_timeTics.front() && pos < 8) {
			painter.drawText(2, 0, 16, 16,
							 Qt::AlignRight | Qt::AlignTop,
							 QString(line.second.c_str()));
			painter.drawText(displayRight + 2, 0, 16, 16,
							 Qt::AlignLeft | Qt::AlignTop,
							 QString(line.second.c_str()));
		} else if (line == m_timeTics.back() && pos > displayBottom - 16) {
			painter.drawText(2, displayBottom - 16, 16, 16,
							 Qt::AlignRight | Qt::AlignBottom,
							 QString(line.second.c_str()));
			painter.drawText(displayRight + 2, displayBottom - 16, 16, 16,
							 Qt::AlignLeft | Qt::AlignBottom,
							 QString(line.second.c_str()));
		} else {
			painter.drawText(2, pos - 8, 16, 16,
							 Qt::AlignRight | Qt::AlignVCenter,
							 QString(line.second.c_str()));
			painter.drawText(displayRight + 2, pos - 8, 16, 16,
							 Qt::AlignLeft | Qt::AlignVCenter,
							 QString(line.second.c_str()));
		}
	}

	// refresh image with new data if needed
	if (m_controls->m_waterfallModel.value() == true) {
		m_processor->m_dataAccess.lock();
		painter.drawImage(displayLeft, 1,
						  QImage(m_processor->m_history.data(),
								 m_processor->binCount(),
								 m_processor->m_waterfallHeight,
								 QImage::Format_RGB32
								 ).scaled(displayWidth,
										  displayBottom - 2,
										  Qt::IgnoreAspectRatio,
										  Qt::SmoothTransformation));
		m_processor->m_dataAccess.unlock();
	}

	// always draw the outline
	painter.setPen(QPen(m_controls->m_colorGrid, 2,
						Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	painter.drawRoundedRect(displayLeft, 1,
							displayWidth, displayBottom,
							2.0, 2.0);

	// dislplay maximum FPS
	#ifdef SA_DEBUG
		start_time = std::chrono::high_resolution_clock::now().time_since_epoch().count() - start_time;
		painter.setPen(QPen(m_controls->m_colorLabels, 1,
							Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
		painter.drawText(displayRight -100, 10, 100, 16, Qt::AlignLeft,
						 QString(std::string("Max FPS: " + std::to_string(1000000000.0 / start_time)).c_str()));
	#endif
}


// Convert time value to Y coordinate for display of given height.
float SaWaterfallView::timeToYPixel(float time, int height) {
	float pixels_per_line = (float)height / m_processor->m_waterfallHeight;
	float seconds_per_line = ((float)m_processor->m_inBlockSize / m_processor->getSampleRate());

	return pixels_per_line * time / seconds_per_line;
}


// Generate labels for linear time scale.
std::vector<std::pair<float, std::string>> SaWaterfallView::makeTimeTics() {
	std::vector<std::pair<float, std::string>> result;
	float i;

	// upper limit defined by number of lines * time per line
	float limit = m_processor->m_waterfallHeight * ((float)m_processor->m_inBlockSize / m_processor->getSampleRate());

	// set increment so that about 8 tics are generated
	float increment = std::round(10 * limit / 7) / 10;

	// NOTE: labels positions are rounded to match the (rounded) label value
	for (i = 0; i <= limit; i += increment) {
		if (i < 10) {
			result.push_back(std::pair<float, std::string>(std::round(i * 10) / 10,
							 std::to_string(std::round(i * 10) / 10).substr(0, 3)));
		} else {
			result.push_back(std::pair<float, std::string>(std::round(i),
							 std::to_string(std::round(i)).substr(0, 2)));
		}
	}
	return result;
}


// Periodically trigger repaint and check if the widget is visible.
// If it is not, the processor can stop processing.
void SaWaterfallView::periodicUpdate() {
	m_processor->setWaterfallActive(isVisible());
	update();
}


// Adjust window size and widget visibility when waterfall is enabled or disabbled.
void SaWaterfallView::updateVisibility() {
	// get container of the control dialog to be resized it if needed
	QWidget *subWindow = m_controls->m_controlsDialog->parentWidget();

	if (m_controls->m_waterfallModel.value()) {
		// clear old data before showing the waterfall
		m_processor->m_dataAccess.lock();
		std::fill(m_processor->m_history.begin(), m_processor->m_history.end(), 0);
		m_processor->m_dataAccess.unlock();

		setVisible(true);

		// increase window size if it is too small
		if (subWindow->size().height() < m_controls->m_controlsDialog->sizeHint().height()) {
			subWindow->resize(subWindow->size().width(), m_controls->m_controlsDialog->sizeHint().height());
		}
	} else {
		setVisible(false);
		// decrease window size only if it does not violate sizeHint
		subWindow->resize(subWindow->size().width(), m_controls->m_controlsDialog->sizeHint().height());
	}
}

