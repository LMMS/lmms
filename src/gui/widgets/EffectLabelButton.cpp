/*
 * EffectLabelButton.cpp - implementation of class trackLabelButton, a label
 *                          which is renamable by double-clicking it
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "EffectLabelButton.h"

#include <QMouseEvent>
#include <QMdiSubWindow>

#include "ConfigManager.h"
#include "embed.h"
#include "EffectView.h"
#include "Effect.h"

namespace lmms::gui
{

EffectLabelButton::EffectLabelButton(EffectView* _tv, QWidget* _parent) :
	QPushButton(_parent),
	m_effectView(_tv),
	m_iconName()
{
	setAttribute(Qt::WA_OpaquePaintEvent, true);
	setAcceptDrops(false);

	setCursor(QCursor(embed::getIconPixmap("hand"), 3, 3));
	setStyleSheet("text-align:left;padding:2px;");
}

void EffectLabelButton::paintEvent(QPaintEvent* pe)
{
	QPushButton::paintEvent(pe);
}

QString EffectLabelButton::elideName(const QString &name)
{
	const int spacing = 16;
	const int maxTextWidth = width() - spacing - iconSize().width();

	setToolTip(name);

	QFontMetrics metrics(font());
	QString elidedName = metrics.elidedText(name, Qt::ElideRight, maxTextWidth);
	return elidedName;
}

} // namespace lmms::gui