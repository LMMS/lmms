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

#include <QImage>
#include <QPainter>


VectorView::VectorView(SaControls *controls, QWidget *_parent) :
	QWidget(_parent),
	m_controls(controls)
{
	setMinimumSize(300, 150);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	connect(gui->mainWindow(), SIGNAL(periodicUpdate()), this, SLOT(periodicUpdate()));

}


// Compose and draw all the content; called by Qt.
void VectorView::paintEvent(QPaintEvent *event)
{
	#ifdef VEC_DEBUG
		int start_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	#endif

	// all drawing done here, local variables are sufficient for the boundary
	const int displayTop = 1;
	const int displayBottom = height() -2;
	const int displayLeft = 26;
	const int displayRight = width() -26;
	const int displayWidth = displayRight - displayLeft;
	const int displayHeight = displayBottom - displayTop;
	float label_width = 20;
	float label_height = 16;
	float margin = 2;

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, true);

	// get buffer from Vectorscope using a lockless FIFO

	// dim stored image based on persistence and elapsed time

	// add new points

	// draw the final image
	painter.drawImage(displayLeft, displayTop,			// top left corner coordinates
					  QImage(m_displayBuffer.data(),	// raw pixel data to display
							 bufferWidth,
							 bufferHeight,
							 QImage::Format_RGB32
							 ).scaled(displayWidth,				// scale to fit view..
									  displayHeight,
									  Qt::IgnoreAspectRatio,
									  Qt::SmoothTransformation));

	// always draw the outline
	painter.setPen(QPen(m_controls->m_colorGrid, 2, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	painter.drawRoundedRect(displayLeft, displayTop, displayWidth, displayBottom, 2.0, 2.0);

	#ifdef VEC_DEBUG
		// display what FPS would be achieved if vectorscope ran in a loop
		start_time = std::chrono::high_resolution_clock::now().time_since_epoch().count() - start_time;
		painter.setPen(QPen(m_controls->m_colorLabels, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
		painter.drawText(displayRight -100, 10, 100, 16, Qt::AlignLeft,
						 QString(std::string("Max FPS: " + std::to_string(1000000000.0 / start_time)).c_str()));
	#endif
}


// Periodically trigger repaint and check if the widget is visible.
void VectorView::periodicUpdate()
{
	m_visible = isVisible();
	if (m_visible) {update();}
}

