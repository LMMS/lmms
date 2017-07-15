/*
 * FxLineLcdSpinBox.cpp - a specialization of LcdSpnBox for setting FX channels
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "FxLineLcdSpinBox.h"

#include "CaptionMenu.h"
#include "FxMixerView.h"
#include "GuiApplication.h"
#include "Track.h"

void FxLineLcdSpinBox::setTrackView(TrackView * tv)
{
	m_tv = tv;
}

void FxLineLcdSpinBox::mouseDoubleClickEvent(QMouseEvent* event)
{
	gui->fxMixerView()->setCurrentFxLine(model()->value());

	gui->fxMixerView()->parentWidget()->show();
	gui->fxMixerView()->show();// show fxMixer window
	gui->fxMixerView()->setFocus();// set focus to fxMixer window
	//engine::getFxMixerView()->raise();
}

void FxLineLcdSpinBox::contextMenuEvent(QContextMenuEvent* event)
{
	// for the case, the user clicked right while pressing left mouse-
	// button, the context-menu appears while mouse-cursor is still hidden
	// and it isn't shown again until user does something which causes
	// an QApplication::restoreOverrideCursor()-call...
	mouseReleaseEvent(nullptr);

	QPointer<CaptionMenu> contextMenu = new CaptionMenu(model()->displayName(), this);

	if (QMenu *fxMenu = m_tv->createFxMenu(
		tr("Assign to:"), tr("New FX Channel")))
	{
		contextMenu->addMenu(fxMenu);

		contextMenu->addSeparator();
	}
	addDefaultActions(contextMenu);
	contextMenu->exec(QCursor::pos());
}
