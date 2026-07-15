/*
 * MainApplication.h - Main QApplication handler
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

#ifndef LMMS_GUI_MAIN_APPLICATION_H
#define LMMS_GUI_MAIN_APPLICATION_H

#include "lmmsconfig.h"

#include <QApplication>

#ifdef LMMS_BUILD_WIN32
#include <windows.h>
#include <QAbstractNativeEventFilter>
#endif


namespace lmms::gui
{


#if defined(LMMS_BUILD_WIN32)
class MainApplication : public QApplication, public QAbstractNativeEventFilter
#else
class MainApplication : public QApplication
#endif
{
public:
	MainApplication(int& argc, char** argv);
	bool event(QEvent* event) override;
#ifdef LMMS_BUILD_WIN32
#if (QT_VERSION < QT_VERSION_CHECK(6,0,0))
	using FilterResult = long;
#else
	using FilterResult = qintptr;
#endif // QT6 check
	bool win32EventFilter(MSG* msg, FilterResult* result);
	bool nativeEventFilter(const QByteArray& eventType, void* message, FilterResult* result);
#endif // LMMS_BUILD_WIN32
	inline QString& queuedFile()
	{
	    return m_queuedFile;
	}
private:
	QString m_queuedFile;
};


} // namespace lmms::gui

#endif // LMMS_GUI_MAIN_APPLICATION_H
