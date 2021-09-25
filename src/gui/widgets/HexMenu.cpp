/*
 * HexMenu.cpp - hexagonal menu widget
 *
 * Copyright (c) 2021 Alex <allejok96/gmail>
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

#include <math.h>
#include <QAction>
#include <QApplication>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QStyleOptionFrameV2>
#include <QTime>

#include "HexMenu.h"
#include "LcdWidget.h"
#include "DeprecationHelper.h"
#include "embed.h"
#include "gui_templates.h"
#include "MainWindow.h"




HexMenu::HexMenu(QWidget* parent) :
	QWidget(parent)
{
	resize(200, 200);
	hide();
}


HexMenu::~HexMenu()
{
}

void HexMenu::addAction(QAction* action)
{
	m_actions.push_back(action);
}

void HexMenu::addMenu(QMenu* menu)
{
	m_contextMenu = menu;
	connect(menu, SIGNAL(aboutToHide()), parentWidget(), SLOT(setFocus()));
}

void HexMenu::mouseReleaseEvent(QMouseEvent* event)
{
	releaseMouse();
	hide();

	if (m_mouseHasMoved)
	{
		m_actions.at(m_hoveredAction)->trigger();
		parentWidget()->setFocus();
	}
	else if (m_contextMenu)
	{
		m_contextMenu->popup(mapToGlobal(event->pos()));
	}

	m_mouseHasMoved = false;
}



void HexMenu::mouseMoveEvent(QMouseEvent *event)
{
	if (!m_mouseHasMoved)
	{
		static const int dragThreshold = 15;
		int dragDistance = std::max(abs(event->x() - rect().center().x()),
									abs(event->y() - rect().center().y()));
		if (dragDistance > dragThreshold)
		{
			m_mouseHasMoved = true;
		}
		else
		{
			return;
		}
	}

	const double pi = 3.1415927;
	QPoint center = rect().center();

	double nearSide = event->x() - center.x();
	double farSide = center.y() - event->y();
	double radians = atan(nearSide / farSide);
	int degrees = radians * 180 / pi;
	if (event->y() > center.y()) { degrees += 180; }

	m_hoveredAction = (degrees + 360) / 60 % 6;

	update();
}



void HexMenu::paintEvent(QPaintEvent*)
{
	if (!m_mouseHasMoved) { return; }

	// load all pixmaps as static so they only load once
	static const QPixmap drawPixmap = embed::getIconPixmap("edit_draw");
	static const QPixmap erasePixmap = embed::getIconPixmap("edit_erase");
	static const QPixmap selectPixmap = embed::getIconPixmap( "edit_select");
	static const QPixmap detunePixmap = embed::getIconPixmap("automation");
	static const QPixmap knifePixmap = embed::getIconPixmap("edit_knife");
	static const QPixmap bulldozerPixmap = embed::getIconPixmap("edit_bulldozer");
	static const QPixmap stampPixmap = embed::getIconPixmap("edit_stamp");
	static const QPixmap strumPixmap = embed::getIconPixmap("edit_strum");

	QPixmap pixmaps[6] {
		strumPixmap,
		drawPixmap,
		knifePixmap,
		detunePixmap,
		erasePixmap,
		bulldozerPixmap
	};

	// Points mapping out an upside-down regular triangle
	double triHeight = width() / 3;
	double triSide = triHeight / (sqrt(3) / 2);
	static const QPointF triangle[3] = {
		QPointF(-triSide / 2, -triHeight),
		QPointF(triSide / 2, -triHeight),
		QPointF(0, 0)
	};

	QColor bgColor(20, 20, 20);
	QColor selColor(50, 50, 50);
	QColor lineColor(127, 127, 127);
	QColor accentColor(0, 127, 0);

	//int side = qMin(width(), height());

	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);
	p.translate(width() / 2, height() / 2);
	//p.scale(side / width(), side / width());
	p.setPen(lineColor);
	p.setBrush(bgColor);

	for (int i = 0; i < 6; ++i) {
		p.save();
		p.rotate(i * 60 + 30);
		p.setBrush(m_hoveredAction == i ? selColor : bgColor);
		p.drawConvexPolygon(triangle, 3);
		if (m_hoveredAction == i)
		{
			p.setPen(QPen(accentColor, 2));
			p.drawLine(triangle[0], triangle[1]);
		}
		p.restore();

		p.save();
		p.rotate(i * 60 + 30);
		p.translate(0, -triHeight * 0.65);
		p.rotate(-i * 60 - 30);
		QPixmap pm = m_actions.at(i)->icon().pixmap(20, 20);
		p.drawPixmap(-pm.width() / 2, -pm.height() / 2, pm);
		p.restore();
	}
}

