/*
 * MainApplication.cpp - Main QApplication handler
 *
 * Copyright (c) 2017-2017 Tres Finocchiaro <tres.finocchiaro/at/gmail.com>
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
#include "MainApplication.h"

#include <QDebug>
#include <QFileOpenEvent>

#include "Engine.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "Song.h"

namespace lmms::gui
{


MainApplication::MainApplication(int& argc, char** argv) :
	QApplication(argc, argv),
	m_queuedFile()
{
#if defined(LMMS_BUILD_WIN32)
	installNativeEventFilter(this);
#endif
}

bool MainApplication::event(QEvent* event)
{
	switch(event->type())
	{
		case QEvent::FileOpen:
		{
			auto fileEvent = static_cast<QFileOpenEvent*>(event);
			// Handle the project file
			m_queuedFile = fileEvent->file();
			if(Engine::getSong())
			{
				if(getGUI()->mainWindow()->mayChangeProject(true))
				{
					qDebug() << "Loading file " << m_queuedFile;
					Engine::getSong()->loadProject(m_queuedFile);
				}
			}
			else
			{
				qDebug() << "Queuing file " << m_queuedFile;
			}
			return true;
		}
		default:
			return QApplication::event(event);
	}
}

#ifdef LMMS_BUILD_WIN32
// This can be moved into nativeEventFilter once Qt4 support has been dropped
bool MainApplication::winEventFilter(MSG* msg, long* result)
{
	switch(msg->message)
	{
		case WM_STYLECHANGING:
			if(msg->wParam == GWL_EXSTYLE)
			{
				// Prevent plugins making the main window transparent
				STYLESTRUCT * style = reinterpret_cast<STYLESTRUCT *>(msg->lParam);
				if(!(style->styleOld & WS_EX_LAYERED))
				{
					style->styleNew &= ~WS_EX_LAYERED;
				}
				*result = 0;
				return true;
			}
			return false;
		default:
			return false;
	}
}

bool MainApplication::nativeEventFilter(const QByteArray& eventType,
					void* message, long* result)
{
	if(eventType == "windows_generic_MSG")
	{
		return winEventFilter(static_cast<MSG *>(message), result);
	}
	return false;
}
#endif // LMMS_BUILD_WIN32


} // namespace lmms::gui
