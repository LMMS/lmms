/*
 * LeftRightNav.cpp - side-by-side left-facing and right-facing arrows for navigation (looks like < > )
 *
 * Copyright (c) 2015 Colin Wallace <wallacoloo/at/gmail.com>
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


#include "LeftRightNav.h"
#include "ToolTip.h"
#include "embed.h"


LeftRightNav::LeftRightNav(QWidget *parent)
 : QWidget(parent),
   m_layout(this),
   m_leftBtn(this, tr("Previous")),
   m_rightBtn(this, tr("Next"))
{
	m_layout.setContentsMargins(0, 0, 0, 0);
	m_layout.setSpacing(2);

	m_leftBtn.setCheckable(false);
	m_rightBtn.setCheckable(false);

	m_leftBtn.setCursor(Qt::PointingHandCursor);
	m_rightBtn.setCursor(Qt::PointingHandCursor);

	m_leftBtn.setActiveGraphic(embed::getIconPixmap(
							"stepper-left-press"));
	m_rightBtn.setActiveGraphic(embed::getIconPixmap(
							"stepper-right-press" ));

	m_leftBtn.setInactiveGraphic(embed::getIconPixmap(
							"stepper-left" ));
	m_rightBtn.setInactiveGraphic(embed::getIconPixmap(
							"stepper-right"));

	connect(&m_leftBtn, SIGNAL(clicked()), this,
						SIGNAL(onNavLeft()));
	connect(&m_rightBtn, SIGNAL(clicked()), this,
						SIGNAL(onNavRight()));

	ToolTip::add(&m_leftBtn, tr("Previous"));
	ToolTip::add(&m_rightBtn, tr("Next"));

	m_leftBtn.setWindowTitle(tr("Previous"));
	m_rightBtn.setWindowTitle(tr("Next"));

	// AutomatableButton's right click menu (contains irrelevant options like copying and pasting values)
	m_leftBtn.setContextMenuPolicy(Qt::NoContextMenu);
	m_rightBtn.setContextMenuPolicy(Qt::NoContextMenu);

	m_layout.addWidget(&m_leftBtn);
	m_layout.addWidget(&m_rightBtn);
}

PixmapButton* LeftRightNav::getLeftBtn()
{
	return &m_leftBtn;
}
PixmapButton* LeftRightNav::getRightBtn()
{
	return &m_rightBtn;
}

void LeftRightNav::setShortcuts(const QKeySequence &leftShortcut, const QKeySequence &rightShortcut)
{
	m_leftBtn.setShortcut(leftShortcut);
	m_rightBtn.setShortcut(rightShortcut);

	ToolTip::add(&m_leftBtn, tr("Previous (%1)").arg(leftShortcut.toString()));
	ToolTip::add(&m_rightBtn, tr("Next (%1)").arg(rightShortcut.toString()));
}