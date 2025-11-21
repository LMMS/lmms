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


#include <QHBoxLayout>

#include "LeftRightNav.h"
#include "embed.h"

namespace lmms::gui
{


LeftRightNav::LeftRightNav(QWidget *parent)
	: QWidget(parent)
	, m_layout(this)
	, m_leftBtn(this)
	, m_rightBtn(this)
{
	m_layout.setContentsMargins(0, 0, 0, 0);
	m_layout.setSpacing(2);

	m_leftBtn.setObjectName("btn-stepper-left");
	m_rightBtn.setObjectName("btn-stepper-right");

	m_leftBtn.setCheckable(false);
	m_rightBtn.setCheckable(false);

	m_leftBtn.setCursor(Qt::PointingHandCursor);
	m_rightBtn.setCursor(Qt::PointingHandCursor);

	connect(&m_leftBtn, SIGNAL(clicked()), this,
						SIGNAL(onNavLeft()));
	connect(&m_rightBtn, SIGNAL(clicked()), this,
						SIGNAL(onNavRight()));

	m_leftBtn.setToolTip(tr("Previous"));
	m_rightBtn.setToolTip(tr("Next"));

	// The context menu contains irrelevant options for these buttons,
	// such as copying and pasting values
	m_leftBtn.setContextMenuPolicy(Qt::NoContextMenu);
	m_rightBtn.setContextMenuPolicy(Qt::NoContextMenu);

	m_layout.addWidget(&m_leftBtn);
	m_layout.addWidget(&m_rightBtn);
}


void LeftRightNav::setShortcuts(const QKeySequence &leftShortcut, const QKeySequence &rightShortcut)
{
	m_leftBtn.setShortcut(leftShortcut);
	m_rightBtn.setShortcut(rightShortcut);

	m_leftBtn.setToolTip(tr("Previous (%1)").arg(leftShortcut.toString()));
	m_rightBtn.setToolTip(tr("Next (%1)").arg(rightShortcut.toString()));
}


} // namespace lmms::gui
