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
#include "SaProcessor.h"

#include "Engine.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "Mixer.h"

SaWaterfallView::SaWaterfallView(SaControls *controls, SaProcessor *processor, QWidget *_parent) :
	QWidget(_parent),
	m_controls(controls),
	m_processor(processor),
	m_periodicalUpdate(false)
{
	setMinimumSize(400, 200);

	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	connect(gui->mainWindow(), SIGNAL(periodicUpdate()), this, SLOT(periodicalUpdate()));

	m_timeTics = makeTimeTics(0, 1);
}


void SaWaterfallView::paintEvent(QPaintEvent *event)
{
	const int displayBottom = height();
	const int displayLeft = 20;
	const int displayRight = width() -20;
	const int displayWidth = displayRight - displayLeft;
	float pos = 0;

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, true);

	painter.setPen(QPen(m_controls->m_colorLabels, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	for (auto & line: m_timeTics) {
		pos = timeToYPixel(line.first, displayBottom);
		// make an exception if the top or bottom label is too close to edge
		if (line == m_timeTics.front() && pos < 8) {
			painter.drawText(2, 0, 16, 16, Qt::AlignRight | Qt::AlignTop, QString(line.second.c_str()));
			painter.drawText(displayRight + 2, 0, 16, 16, Qt::AlignLeft | Qt::AlignTop, QString(line.second.c_str()));
		} else if (line == m_timeTics.back() && pos > displayBottom - 16) {
			painter.drawText(2, displayBottom - 16, 16, 16, Qt::AlignRight | Qt::AlignBottom, QString(line.second.c_str()));
			painter.drawText(displayRight + 2, displayBottom - 16, 16, 16, Qt::AlignLeft | Qt::AlignBottom, QString(line.second.c_str()));
		} else {
			painter.drawText(2, pos - 8, 16, 16, Qt::AlignRight | Qt::AlignVCenter, QString(line.second.c_str()));
			painter.drawText(displayRight + 2, pos - 8, 16, 16, Qt::AlignLeft | Qt::AlignVCenter, QString(line.second.c_str()));
		}
	}

	// draw waterfallView only if enabled and ... if there is any new signal?			FIXME
		// stop after signal disappears, or just roll-off with zeros and _then_ stop?
		// also, make sure the history buffer is not being updated while drawing (possibly cause of the flicker?
			// maybe use simple double buffering?

	if (m_controls->m_waterfallModel.value() == true) {

		// refresh image with new data if needed
		if (!m_processor->getInProgress() && m_periodicalUpdate == true) {
			m_periodicalUpdate = false;

			painter.drawImage(displayLeft, 1, QImage(m_processor->m_history.data(), WATERFALL_WIDTH, WATERFALL_HEIGHT, QImage::Format_RGB32).scaled(displayWidth, displayBottom - 2, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
		}
	
	}

	// always draw the outline
	painter.setPen(QPen(m_controls->m_colorGrid, 2, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	painter.drawRoundedRect(displayLeft, 1, displayWidth, displayBottom, 2.0, 2.0);

}


float SaWaterfallView::timeToYPixel(float time, int height)
{
	return height * time;
}


std::vector<std::pair<float, std::string>> SaWaterfallView::makeTimeTics(int low, int high)
{
	std::vector<std::pair<float, std::string>> result;
	float i;

	// generate 0.2 linear amplitude increments
	for (i = 0; i <= high; i += 0.1){
		if (i >= low){
			result.push_back(std::pair<float, std::string>(i, std::to_string(i).substr(0, 3)));
		}
	}

	return result;
}


void SaWaterfallView::periodicalUpdate()
{
	m_periodicalUpdate = true;
	m_processor->setActive(isVisible());
	update();
}
