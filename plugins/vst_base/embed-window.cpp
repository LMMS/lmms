/*
 * embed-window.cpp - simple application that embeds an external window
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

#include "EmbedderApplication.h"

#include <QSocketNotifier>
#include <QTimer>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#if QT_VERSION < 0x050000
#include <QX11EmbedContainer>
#include <QX11Info>
#include <X11/Xlib.h>
#else
#include <QWindow>
#endif


void MainWindow::init( const char * title, unsigned int windowId, int width,
								int height )
{
	setWindowTitle( title );
#if QT_VERSION < 0x050000
	m_window = new QX11EmbedContainer( this );
	QX11EmbedContainer * container = m_window;
	container->embedClient( windowId );
#else
	m_window = QWindow::fromWinId( windowId );
	QWidget * container = QWidget::createWindowContainer( m_window, this );
#endif
	container->setFixedSize( width, height );
	container->show();
	setFixedSize( width, height );
}




void MainWindow::closeEvent( QCloseEvent *event )
{
	hide();
#if QT_VERSION < 0x050000
	XUnmapWindow( QX11Info::display(), m_window->clientWinId() );
	m_window->discardClient();
#else
	m_window->setParent( 0 );
#endif
	QMainWindow::closeEvent( event );
}




EmbedderApplication::EmbedderApplication( int & argc, char * * argv ) :
	QApplication( argc, argv )
{
	QSocketNotifier * notifier = new QSocketNotifier( STDIN_FILENO,
							QSocketNotifier::Read );
	connect( notifier, SIGNAL( activated( int ) ), SLOT( readCommand() ) );
}




EmbedderApplication::~EmbedderApplication()
{
}




void EmbedderApplication::init( const char * title, unsigned int windowId,
							int width, int height )
{
	m_mainWindow.init( title, windowId, width, height );
}




void EmbedderApplication::applicationReady()
{
	putchar( 0 );
	fflush( stdout );
}




void EmbedderApplication::readCommand()
{
	int c = getchar();
	if( c == EOF )
	{
		m_mainWindow.close();
		quit();
		return;
	}
	m_mainWindow.show();
}




int main( int argc, char * * argv )
{
	if( argc < 5 )
	{
		fputs( "Missing arguments\n", stderr );
		return EXIT_FAILURE;
	}

	EmbedderApplication * app = new EmbedderApplication( argc, argv );

	const char * title = argv[1];
	unsigned int windowId = strtol( argv[2], NULL, 16 );
	int width = strtol( argv[3], NULL, 16 );
	int height = strtol( argv[4], NULL, 16 );

	app->init( title, windowId, width, height );

	QTimer::singleShot( 0, app, SLOT( applicationReady() ) );

	int ret = app->exec();
	delete app;

	return ret;
}
