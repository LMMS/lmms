/*
 * DetachableWidget.cpp - Allows a widget to be detached from
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

#include "DetachableWidget.h"

#include <QEvent>

#include "GuiApplication.h"
#include "MainWindow.h"

namespace lmms::gui {


bool DetachableWidget::eventFilter(QObject* obj, QEvent* e)
{
	if (!obj->isWidgetType())
		return false;
	auto w = static_cast<QWidget*>(obj);
	if (e->type() == QEvent::Close)
	{
		if (w->windowFlags().testFlag(Qt::Window))
		{
			emit attach();  // we'll handle this in SubWinoow
			e->ignore();
			return true;
		}
		else if (getGUI()->mainWindow()->workspace())  // mdiArea exists
		{
			w->parentWidget()->hide();
			e->ignore();
		}
		else  // is this even reachable?
		{
			w->hide();
			e->ignore();
		}
		return false;
	}
	return false;
}


} // namespace lmms::gui
