/*
 * EffectLabelButton.cpp - implementation of class trackLabelButton, a label
 *                          which is renamable by double-clicking it
 *
 * Copyright (c) 2024 Noah Brecht <noahb2713/at/gmail/dot/com>
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
	m_effectView(_tv)
{
	setAttribute(Qt::WA_OpaquePaintEvent, true);
	setAcceptDrops(false);

	setCursor(QCursor(embed::getIconPixmap("hand"), 3, 3));
	setStyleSheet("text-align:left;padding:2px;");
	setCheckable(true);
	setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);
}

} // namespace lmms::gui
