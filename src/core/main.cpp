/*
 * main.cpp - just main.cpp which is starting up app...
 *
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#include "qt3support.h"

#ifdef QT4

#include <QApplication>
#include <QLocale>
#include <QSplashScreen>

#else

#include <qapplication.h>
#include <qtextcodec.h>
#if QT_VERSION >= 0x030200
#include <qsplashscreen.h>
#endif

#endif

#include "lmms_main_win.h"
#include "embed.h"
#include "config_mgr.h"
#include "export_project_dialog.h"
#include "song_editor.h"
#include "gui_templates.h"


#if QT_VERSION >= 0x030100
int splash_alignment_flags = Qt::AlignTop | Qt::AlignLeft;
#endif

QString file_to_load;
QString file_to_render;


int main( int argc, char * * argv )
{
	QApplication app( argc, argv );

	for( int i = 1; i < app.argc(); ++i )
	{
		if( QString( app.argv()[i] ) == "--version" ||
					QString( app.argv()[i] ) == "-v" )
		{
			printf( "\n%s\n\n"
	"This program is free software; you can redistribute it and/or\n"
	"modify it under the terms of the GNU General Public\n"
	"License as published by the Free Software Foundation; either\n"
	"version 2 of the License, or (at your option) any later version.\n\n",
							PACKAGE_STRING );
			return( 0 );
		}
		else if( app.argc() > i &&
				( QString( app.argv()[i] ) == "--render" ||
					QString( app.argv()[i] ) == "-r" ) )
		{
			file_to_load = QString( app.argv()[i+1] );
			file_to_render = QString( app.argv()[i+1] ) + ".wav";
		}
		else
		{
			file_to_load = app.argv()[i];
		}
	}

	QString pos =
#ifdef QT4
			QLocale::system().name().left( 2 );
#else
			QString( QTextCodec::locale() ).section( '_', 0, 0 );
#endif
	// load translation for Qt-widgets/-dialogs
	embed::loadTranslation( QString( "qt_" ) + pos );
	// load actual translation for LMMS
	embed::loadTranslation( pos );

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
	pal.setColor( QPalette::Highlight, QColor( 255, 224, 224 ) );
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
	lmmsMainWin::s_splashScreen = new QSplashScreen( splash );
	lmmsMainWin::s_splashScreen->show();

	lmmsMainWin::s_splashScreen->showMessage( lmmsMainWin::tr(
							"Setting up main-"
							"window and "
							"workspace..." ),
							splash_alignment_flags,
							Qt::white );
#endif

#ifndef QT4
	app.setMainWidget( lmmsMainWin::inst() );
#endif
	// MDI-mode?
	if( lmmsMainWin::inst()->workspace() != NULL )
	{
		// then maximize
		lmmsMainWin::inst()->showMaximized();
	}
	else
	{
		// otherwise arrange at top-left edge of screen
		lmmsMainWin::inst()->show();
		lmmsMainWin::inst()->move( 0, 0 );
		lmmsMainWin::inst()->resize( 200, 500 );
	}


#if QT_VERSION >= 0x030200
	delete lmmsMainWin::s_splashScreen;
	lmmsMainWin::s_splashScreen = NULL;
#endif
	if( file_to_render != "" )
	{
		exportProjectDialog * e = new exportProjectDialog(
							file_to_render,
							lmmsMainWin::inst() );
		songEditor::inst()->setExportProjectDialog( e );
		e->show();
		e->exportBtnClicked();
	}

	return( app.exec() );
}

