#ifndef SINGLE_SOURCE_COMPILE

/*
 * main.cpp - just main.cpp which is starting up app...
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "qt3support.h"

#ifdef QT4

#include <QtCore/QLocale>
#include <QtCore/QTranslator>
#include <QtGui/QApplication>
#include <QtGui/QSplashScreen>

#else

#include <qapplication.h>
#include <qtextcodec.h>
#if QT_VERSION >= 0x030200
#include <qsplashscreen.h>
#endif
#include <qtranslator.h>

#endif

#ifdef HAVE_SCHED_H
#include <sched.h>
#endif


#include "main_window.h"
#include "embed.h"
#include "config_mgr.h"
#include "export_project_dialog.h"
#include "song_editor.h"
#include "gui_templates.h"


QString file_to_render;


#if QT_VERSION >= 0x030100
int splash_alignment_flags = Qt::AlignTop | Qt::AlignLeft;
#endif

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
	QString file_to_load;

	for( int i = 1; i < app.argc(); ++i )
	{
		if( QString( app.argv()[i] ) == "--version" ||
					QString( app.argv()[i] ) == "-v" )
		{
			printf( "\nLinux MultiMedia Studio %s\n\n"
	"Copyright (c) 2004-2006 Tobias Doerffel and others.\n\n"
	"This program is free software; you can redistribute it and/or\n"
	"modify it under the terms of the GNU General Public\n"
	"License as published by the Free Software Foundation; either\n"
	"version 2 of the License, or (at your option) any later version.\n\n"
	"Try \"%s --help\" for more information.\n\n", PACKAGE_VERSION,
								argv[0] );
			return( 0 );
		}
		else if( app.argc() > i &&
				( QString( app.argv()[i] ) == "--help" ||
					QString( app.argv()[i] ) == "-h" ) )
		{
			printf( "\nLinux MultiMedia Studio %s\n"
	"Copyright (c) 2004-2006 Tobias Doerffel and others.\n\n"
	"usage: lmms [ -r <file_to_render> [ -o <format> ] [ -h ] "
							"[ <file_to_load> ]\n"
	"-r, --render			render given file.\n"
	"-o, --output-format <format>	specify format of render-output where\n"
	"				format is either 'wav' or 'ogg'.\n"
	"-v, --version			show version information and exit.\n"
	"-h, --help			show this usage message and exit.\n\n",
							PACKAGE_VERSION );
			return( 0 );
		}
		else if( app.argc() > i &&
				( QString( app.argv()[i] ) == "--render" ||
					QString( app.argv()[i] ) == "-r" ) )
		{
			file_to_load = QString( app.argv()[i+1] );
			file_to_render = baseName( file_to_load ) + ".";
			++i;
		}
		else if( app.argc() > i &&
			( QString( app.argv()[i] ) == "--output-format" ||
					QString( app.argv()[i] ) == "-o" ) )
		{
			extension = QString( app.argv()[i+1] );
			if( extension != "wav" && extension != "ogg" )
			{
				printf( "\nInvalid output format %s.\n\n"
	"Try \"%s --help\" for more information.\n\n", app.argv()[i+1],
								argv[0] );
				return( -1 );
			}
			++i;
		}
		else
		{
			if( app.argv()[i][0] == '-' )
			{
				printf( "\nInvalid option %s.\n\n"
	"Try \"%s --help\" for more information.\n\n", app.argv()[i],
								argv[0] );
				return( -1 );
			}
			file_to_load = app.argv()[i];
		}
	}

	if( file_to_render != "" )
	{
		file_to_render += extension;
	}


	QString pos =
#ifdef QT4
			QLocale::system().name().left( 2 );
#else
			QString( QTextCodec::locale() ).section( '_', 0, 0 );
#endif
	// load translation for Qt-widgets/-dialogs
#ifdef QT_TRANSLATIONS_DIR
	loadTranslation( QString( "qt_" ) + pos,
					QString( QT_TRANSLATIONS_DIR ) );
#else
	loadTranslation( QString( "qt_" ) + pos );
#endif
	// load actual translation for LMMS
	loadTranslation( pos );

#ifdef QT4
	app.setFont( pointSize<10>( app.font() ) );
#endif


	if( !configManager::inst()->loadConfigFile() )
	{
		return( -1 );
	}

	QPalette pal = app.palette();
#ifdef QT4
	pal.setColor( QPalette::Background, QColor( 128, 128, 128 ) );
	pal.setColor( QPalette::Foreground, QColor( 240, 240, 240 ) );
	pal.setColor( QPalette::Base, QColor( 128, 128, 128 ) );
	pal.setColor( QPalette::Text, QColor( 224, 224, 224 ) );
	pal.setColor( QPalette::Button, QColor( 160, 160, 160 ) );
	pal.setColor( QPalette::ButtonText, QColor( 255, 255, 255 ) );
	pal.setColor( QPalette::Highlight, QColor( 224, 224, 224 ) );
	pal.setColor( QPalette::HighlightedText, QColor( 0, 0, 0 ) );
#else
	pal.setColor( QColorGroup::Background, QColor( 128, 128, 128 ) );
	pal.setColor( QColorGroup::Foreground, QColor( 240, 240, 240 ) );
	pal.setColor( QColorGroup::Base, QColor( 128, 128, 128 ) );
	pal.setColor( QColorGroup::Text, QColor( 224, 224, 224 ) );
	pal.setColor( QColorGroup::Button, QColor( 160, 160, 160 ) );
	pal.setColor( QColorGroup::ButtonText, QColor( 255, 255, 255 ) );
	pal.setColor( QColorGroup::Highlight, QColor( 224, 224, 224 ) );
	pal.setColor( QColorGroup::HighlightedText, QColor( 0, 0, 0 ) );
#endif
	app.setPalette( pal );


#if QT_VERSION >= 0x030200
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
#endif

	engine * main_engine = new engine();

	// we try to load given file
	if( file_to_load != "" )
	{
		main_engine->getSongEditor()->loadProject( file_to_load );
	}
	else
	{
		main_engine->getSongEditor()->createNewProject();
	}
#ifndef QT4
	app.setMainWidget( main_engine->getMainWindow() );
#endif
	// MDI-mode?
	if( main_engine->getMainWindow()->workspace() != NULL )
	{
		// then maximize
		main_engine->getMainWindow()->showMaximized();
	}
	else
	{
		// otherwise arrange at top-left edge of screen
		main_engine->getMainWindow()->show();
		main_engine->getMainWindow()->move( 0, 0 );
		main_engine->getMainWindow()->resize( 200, 500 );
	}


#if QT_VERSION >= 0x030200
	delete mainWindow::s_splashScreen;
	mainWindow::s_splashScreen = NULL;
#endif
	if( file_to_render != "" )
	{
		exportProjectDialog * e = new exportProjectDialog(
						file_to_render,
						main_engine->getMainWindow(),
						main_engine );
		e->show();
		e->exportBtnClicked();
	}

	return( app.exec() );
}


#endif
