/*
 * DetachableWindow.cpp - Allows a window to be detached from
 *                        LMMS's main window
 *
 * Copyright (c) 2023 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#include "DetachableWindow.h"

#include <QCloseEvent>

#include "GuiApplication.h"
#include "MainWindow.h"

namespace lmms::gui {

void DetachableWindow::closeEvent(QCloseEvent* ce)
{
	if (windowFlags().testFlag(Qt::Window))
	{
		ce->accept();
	}
	else if (getGUI()->mainWindow()->workspace())
	{
		parentWidget()->hide();
		ce->ignore();
	}
	else
	{
		hide();
		ce->ignore();
	}
	emit closed();
}

} // namespace lmms::gui
