/*
 * DetachedWindow.cpp - Substitute for SubWindow.cpp to be used when window is detached.
 *
 * Copyright (c) 2015 Colin Wallace <wallace.colin.a@gmail.com>
 * Copyright (c) 2016 Steffen Baranowsky <baramgb@freenet.de>
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

#include "DetachedWindow.h"
#include "SubWindow.h"

#include <QLayout>

namespace lmms::gui
{


DetachedWindow::DetachedWindow(QWidget *child, QWidget *parent, Qt::WindowFlags windowFlags) :
	QWidget(parent, windowFlags)
{
	m_layout = new QVBoxLayout(this);
	m_layout->setSizeConstraint(QLayout::SetMinAndMaxSize);  // Sync min & max size with child
	m_layout->setContentsMargins(0, 0, 0, 0);  // Remove default margins, making the container effectively invisible
	
    setWidget(child);  // does nothing if child is nullptr, additional checks unneessary

	setWindowFlags((windowFlags & ~Qt::Widget) | Qt::Window);  // make the container a detached window
}


void DetachedWindow::closeEvent(QCloseEvent* ce)
{
	attach();
    // This is our current method of attaching the window,
	// and invokes destruction of the widget.
	// If replaced by any other method (a GUI button or otherwise), this would be safe to remove.
	// Making this contain `hide(); ce.ignore();` would make the window persist in its detached state,
	// mirroring behavior of SubWindow.
}

bool DetachedWindow::isDetached() const
{
	return true;
}

void DetachedWindow::setWidget(QWidget* w)
{
	if (widget())
		m_layout->removeWidget(widget());
	if (w)
		m_layout->addWidget(w);
}

QWidget* DetachedWindow::widget() const
{
	// It could be better to keep pointer to child a variable and return it,
	// but for the case with no decorations this works too.

	if (m_layout->count())
		return m_layout->itemAt(0)->widget();
	return nullptr;
}

void DetachedWindow::detach()
{
	// dynamic_cast<SubWindow&>(*parentWidget()).detach();
	// Implementation in SubWindow
	
	// Does nothing since existence of this object already implies that window is detached,
	// making the parent function a no-op. Kept for compliance
	return;
}

void DetachedWindow::attach()
{
	dynamic_cast<SubWindow&>(*parentWidget()).attach();
	// Implementation in SubWindow
}

} // namespace lmms::gui
