/*
 * EmbedderApplication.h - simple application that embeds an external window
 *
 * Copyright (c) 2016 Javier Serrano Polo <javier@jasp.net>
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

#ifndef EMBEDDER_APPLICATION_H
#define EMBEDDER_APPLICATION_H

#include <QApplication>
#include <QMainWindow>

#if QT_VERSION < 0x050000
class QX11EmbedContainer;
#endif


class MainWindow : public QMainWindow
{
public:
	void init( const char * title, unsigned int windowId, int width,
								int height );


protected:
	virtual void closeEvent( QCloseEvent *event );


private:
#if QT_VERSION < 0x050000
	QX11EmbedContainer * m_window;
#else
	QWindow * m_window;
#endif

} ;




class EmbedderApplication : public QApplication
{
	Q_OBJECT
public:
	EmbedderApplication( int & argc, char * * argv );

	virtual ~EmbedderApplication();

	void init( const char * title, unsigned int windowId, int width,
								int height );


private:
	MainWindow m_mainWindow;


private slots:
	void applicationReady();
	void readCommand();

} ;


#endif
