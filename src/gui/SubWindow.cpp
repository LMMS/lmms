/*
 * SubWindow.cpp - Implementation of QMdiSubWindow that correctly tracks
 *   the geometry that windows should be restored to.
 *   Workaround for https://bugreports.qt.io/browse/QTBUG-256
 *
 * Copyright (c) 2015 Colin Wallace <wallace.colin.a@gmail.com>
 *
 * This file is part of LMMS - http://lmms.io
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

#include "SubWindow.h"

#include <QMoveEvent>
#include <QResizeEvent>
#include <QWidget>


SubWindow::SubWindow(QWidget *parent, Qt::WindowFlags windowFlags)
  : QMdiSubWindow(parent, windowFlags)
{
	// initialize the tracked geometry to whatever Qt thinks the normal geometry currently is.
	// this should always work, since QMdiSubWindows will not start as maximized
	m_trackedNormalGeom = normalGeometry();
}

QRect SubWindow::getTrueNormalGeometry() const
{
	return m_trackedNormalGeom;
}

void SubWindow::moveEvent(QMoveEvent * event)
{
	QMdiSubWindow::moveEvent(event);
	// if the window was moved and ISN'T minimized/maximized/fullscreen,
	//   then save the current position
	if (!isMaximized() && !isMinimized() && !isFullScreen())
	{
		m_trackedNormalGeom.moveTopLeft(event->pos());
	}
}

void SubWindow::resizeEvent(QResizeEvent * event)
{
	QMdiSubWindow::resizeEvent(event);
	// if the window was resized and ISN'T minimized/maximized/fullscreen,
	//   then save the current size
	if (!isMaximized() && !isMinimized() && !isFullScreen())
	{
		m_trackedNormalGeom.setSize(event->size());
	}
}