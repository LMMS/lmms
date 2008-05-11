#ifndef SINGLE_SOURCE_COMPILE

/*
 * main.cpp - just main.cpp which is starting up app...
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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


#include <QtCore/QFileInfo>
#include <QtCore/QLocale>
#include <QtCore/QTime>
#include <QtCore/QTranslator>
#include <QtGui/QApplication>
#include <QtGui/QSplashScreen>

#ifdef HAVE_SCHED_H
#include <sched.h>
#endif


#include "main_window.h"
#include "embed.h"
#include "engine.h"
#include "config_mgr.h"
#include "project_renderer.h"
#include "song.h"
#include "gui_templates.h"
#include "lmms_style.h"

#warning TODO: move somewhere else
static inline QString baseName( const QString & _file )
{
	return( QFileInfo( _file ).absolutePath() + "/" +
			QFileInfo( _file ).completeBaseName() );
}


int splash_alignment_flags = Qt::AlignTop | Qt::AlignLeft;

inline void loadTranslation( const QString & _tname,
	const QString & _dir = configManager::inst()->localeDir() )
{
	QTranslator * t = new QTranslator( 0 );
	QString name = _tname + ".qm";

	t->load( name, _dir );

	qApp->installTranslator( t );
}



int main( int argc, char * * argv )
{
#ifdef HAVE_SCHED_H
	struct sched_param sparam;
	sparam.sched_priority = ( sched_get_priority_max( SCHED_FIFO ) +
				sched_get_priority_min( SCHED_FIFO ) ) / 2;
	if( sched_setscheduler( 0, SCHED_FIFO, &sparam ) == -1 )
	{
		printf( "could not set realtime priority.\n" );
	}
#endif

	QApplication app( argc, argv );

	QString extension = "wav";
	QString file_to_load, file_to_render;

	for( int i = 1; i < argc; ++i )
	{
		if( QString( argv[i] ) == "--version" ||
						QString( argv[i] ) == "-v" )
		{
			printf( "\nLinux MultiMedia Studio %s\n\n"
	"Copyright (c) 2004-2008 LMMS developers.\n\n"
	"This program is free software; you can redistribute it and/or\n"
	"modify it under the terms of the GNU General Public\n"
	"License as published by the Free Software Foundation; either\n"
	"version 2 of the License, or (at your option) any later version.\n\n"
	"Try \"%s --help\" for more information.\n\n", PACKAGE_VERSION,
								argv[0] );
			return( EXIT_SUCCESS );
		}
		else if( argc > i && ( QString( argv[i] ) == "--help" ||
						QString( argv[i] ) == "-h" ) )
		{
			printf( "\nLinux MultiMedia Studio %s\n"
	"Copyright (c) 2004-2008 LMMS developers.\n\n"
	"usage: lmms [ -r <file_to_render> [ -o <format> ] [ -h ] "
							"[ <file_to_load> ]\n"
	"-r, --render			render given file.\n"
	"-o, --output-format <format>	specify format of render-output where\n"
	"				format is either 'wav' or 'ogg'.\n"
	"-v, --version			show version information and exit.\n"
	"-h, --help			show this usage message and exit.\n\n",
							PACKAGE_VERSION );
			return( EXIT_SUCCESS );
		}
		else if( argc > i && ( QString( argv[i] ) == "--render" ||
						QString( argv[i] ) == "-r" ) )
		{
			file_to_load = QString( argv[i + 1] );
			file_to_render = baseName( file_to_load ) + ".";
			++i;
		}
		else if( argc > i &&
				( QString( argv[i] ) == "--output-format" ||
						QString( argv[i] ) == "-o" ) )
		{
			extension = QString( argv[i + 1] );
			if( extension != "wav" && extension != "ogg" )
			{
				printf( "\nInvalid output format %s.\n\n"
	"Try \"%s --help\" for more information.\n\n", argv[i + 1], argv[0] );
				return( EXIT_FAILURE );
			}
			++i;
		}
		else
		{
			if( argv[i][0] == '-' )
			{
				printf( "\nInvalid option %s.\n\n"
	"Try \"%s --help\" for more information.\n\n", argv[i], argv[0] );
				return( EXIT_FAILURE );
			}
			file_to_load = argv[i];
		}
	}

	bool gui_startup = TRUE;
	if( file_to_render != "" )
	{
		file_to_render += extension;
		gui_startup = FALSE;
	}


	QString pos = QLocale::system().name().left( 2 );
	// load translation for Qt-widgets/-dialogs
	loadTranslation( QString( "qt_" ) + pos,
					QString( QT_TRANSLATIONS_DIR ) );
	// load actual translation for LMMS
	loadTranslation( pos );

//	app.setFont( pointSize<10>( app.font() ) );


	if( !configManager::inst()->loadConfigFile() )
	{
		return( EXIT_FAILURE );
	}

	if( gui_startup )
	{
		QApplication::setStyle( new lmmsStyle() );

		// set palette
		QPalette pal = app.palette();
		//pal.setColor( QPalette::Background, QColor( 72, 76 ,88 ) );
		pal.setColor( QPalette::Background, QColor( 72, 76, 88 ) );
		//pal.setColor( QPalette::Background, QColor( 127, 134 ,154 ) );
		//pal.setColor( QPalette::Background, QColor( 105, 110, 120 ) );
		//pal.setColor( QPalette::Background, QColor( 89, 99, 128 ) );
		
		pal.setColor( QPalette::Foreground, QColor( 240, 240, 240 ) );
		
		pal.setColor( QPalette::Base, QColor( 128, 128, 128 ) );
		//pal.setColor( QPalette::Base, QColor( 115, 110 , 94 ) );
		//pal.setColor( QPalette::Base, QColor( 128, 112 , 94 ) );
		
		pal.setColor( QPalette::Text, QColor( 224, 224, 224 ) );
	
		pal.setColor( QPalette::Button, QColor( 172, 176, 188 ) );
		//pal.setColor( QPalette::Button, QColor( 172, 177, 191 ) );
		
		pal.setColor( QPalette::ButtonText, QColor( 255, 255, 255 ) );
		pal.setColor( QPalette::Highlight, QColor( 224, 224, 224 ) );
		pal.setColor( QPalette::HighlightedText, QColor( 0, 0, 0 ) );
		app.setPalette( pal );


		// init splash screen
		QPixmap splash = embed::getIconPixmap( "splash" );
		mainWindow::s_splashScreen = new QSplashScreen( splash );
		mainWindow::s_splashScreen->show();

		mainWindow::s_splashScreen->showMessage( mainWindow::tr(
								"Setting up main-"
								"window and "
								"workspace..." ),
								splash_alignment_flags,
								Qt::white );

		engine::init();

		// we try to load given file
		if( file_to_load != "" )
		{
			engine::getSong()->loadProject( file_to_load );
		}
		else
		{
			engine::getSong()->createNewProject();
		}

		// MDI-mode?
		if( engine::getMainWindow()->workspace() != NULL )
		{
			// then maximize
			engine::getMainWindow()->showMaximized();
		}
		else
		{
			// otherwise arrange at top-left edge of screen
			engine::getMainWindow()->show();
			engine::getMainWindow()->move( 0, 0 );
			engine::getMainWindow()->resize( 200, 500 );
		}


		delete mainWindow::s_splashScreen;
		mainWindow::s_splashScreen = NULL;
	}
	else
	{
		engine::init( FALSE );
		engine::getSong()->loadProject( file_to_load );
/*		exportProjectDialog * e = new exportProjectDialog(
						file_to_render,
						engine::getMainWindow() );
		e->show();
		QTime t;
		t.start();
		e->exportBtnClicked();
		printf("export took %d ms\n",t.elapsed());*/
	}

	return( app.exec() );
}


#endif
