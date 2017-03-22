/*
 * MainApplication.h - Main QApplication handler
 *
 * Copyright (c) 2017-2017 Tres Finocchiaro <tres.finocchiaro/at/gmail.com>
 *
 * This file is part of LMMS - http://lmms.io
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

#ifndef MAINAPPLICATION_H
#define MAINAPPLICATION_H

#include <QApplication>

class MainApplication : public QApplication
{
public:
	MainApplication(int& argc, char** argv);
	bool event(QEvent* event);
	inline QString& queuedFile()
	{
	    return m_queuedFile;
	}
private:
	QString m_queuedFile;
};

#endif // MAINAPPLICATION_H
