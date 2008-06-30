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
#include <QtCore/QTimer>
#include <QtCore/QTranslator>
#include <QtGui/QApplication>
#include <QtGui/QBitmap>
#include <QtGui/QDesktopWidget>
#include <QtGui/QPainter>
#include <QtGui/QSplashScreen>

#ifdef LMMS_HAVE_SCHED_H
#include <sched.h>
#endif


#include "main_window.h"
#include "embed.h"
#include "engine.h"
#include "config_mgr.h"
#include "project_renderer.h"
#include "song.h"
#include "lmms_style.h"

#warning TODO: move somewhere else
static inline QString baseName( const QString & _file )
{
	return( QFileInfo( _file ).absolutePath() + "/" +
			QFileInfo( _file ).completeBaseName() );
}


inline void loadTranslation( const QString & _tname,
	const QString & _dir = configManager::inst()->localeDir() )
{
	QTranslator * t = new QTranslator( QCoreApplication::instance() );
	QString name = _tname + ".qm";

	t->load( name, _dir );

	QCoreApplication::instance()->installTranslator( t );
}



int main( int argc, char * * argv )
{
#ifdef LMMS_HAVE_SCHED_H
	struct sched_param sparam;
	sparam.sched_priority = ( sched_get_priority_max( SCHED_FIFO ) +
				sched_get_priority_min( SCHED_FIFO ) ) / 2;
	if( sched_setscheduler( 0, SCHED_FIFO, &sparam ) == -1 )
	{
		printf( "could not set realtime priority.\n" );
	}
#endif

	bool core_only = FALSE;

	for( int i = 1; i < argc; ++i )
	{
		if( argc > i && ( QString( argv[i] ) == "--render" ||
					QString( argv[i] ) == "-r" ) ||
				( QString( argv[i] ) == "--help" ||
						QString( argv[i] ) == "-h" ) )
		{
			core_only = TRUE;
			break;
		}
	}

	QCoreApplication * app = core_only ?
			new QCoreApplication( argc, argv ) :
					new QApplication( argc, argv ) ;

	QString file_to_load, render_out;

	mixer::qualitySettings qs( mixer::qualitySettings::Mode_HighQuality );
	projectRenderer::outputSettings os( 44100, FALSE, 160,
						projectRenderer::Depth_16Bit );
	projectRenderer::ExportFileFormats eff = projectRenderer::WaveFile;


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
	"Try \"%s --help\" for more information.\n\n", LMMS_PACKAGE_VERSION,
								argv[0] );
			return( EXIT_SUCCESS );
		}
		else if( argc > i && ( QString( argv[i] ) == "--help" ||
						QString( argv[i] ) == "-h" ) )
		{
			printf( "\nLinux MultiMedia Studio %s\n"
	"Copyright (c) 2004-2008 LMMS developers.\n\n"
	"usage: lmms [ -r <project file> [ -f <format> ] [ -s <samplerate> ]\n"
	"            [ -b <bitrate> ] [ -i <method> ] [ -x <value> ] [ -h ]\n"
	"	     [ <file to load> ]\n\n"
	"-r, --render <project file>	render given project file\n"
	"-o, --output <file>		render into <file>\n"
	"-f, --output-format <format>	specify format of render-output where\n"
	"				format is either 'wav' or 'ogg'.\n"
	"-s, --samplerate <samplerate>	specify output samplerate in Hz\n"
	"				range: 44100 to 192000\n"
	"				default: 44100.\n"
	"-b, --bitrate <bitrate>	specify output bitrate in kHz\n"
	"				default: 160.\n"
	"-i, --interpolation <method>	specify interpolation method\n"
	"				possible values:\n"
	"				   - linear.\n"
	"				   - sincfastest.\n"
	"				   - sincmedium.\n"
	"				   - sincbest.\n"
	"				default: sincfastest.\n"
	"-x, --oversampling <value>	specify oversampling\n"
	"				possible values: 1, 2, 4, 8\n"
	"				default: 2\n"
	"-v, --version			show version information and exit.\n"
	"-h, --help			show this usage message and exit.\n\n",
							LMMS_PACKAGE_VERSION );
			return( EXIT_SUCCESS );
		}
		else if( argc > i && ( QString( argv[i] ) == "--render" ||
						QString( argv[i] ) == "-r" ) )
		{
			file_to_load = QString( argv[i + 1] );
			render_out = baseName( file_to_load ) + ".";
			++i;
		}
		else if( argc > i && ( QString( argv[i] ) == "--output" ||
						QString( argv[i] ) == "-o" ) )
		{
			render_out = baseName( QString( argv[i + 1] ) ) + ".";
			++i;
		}
		else if( argc > i &&
				( QString( argv[i] ) == "--format" ||
						QString( argv[i] ) == "-f" ) )
		{
			const QString ext = QString( argv[i + 1] );
			if( ext == "wav" )
			{
				eff = projectRenderer::WaveFile;
			}
			else if( ext == "ogg" )
			{
				eff = projectRenderer::OggFile;
			}
			else
			{
				printf( "\nInvalid output format %s.\n\n"
	"Try \"%s --help\" for more information.\n\n", argv[i + 1], argv[0] );
				return( EXIT_FAILURE );
			}
			++i;
		}
		else if( argc > i &&
				( QString( argv[i] ) == "--samplerate" ||
						QString( argv[i] ) == "-s" ) )
		{
			sample_rate_t sr = QString( argv[i + 1] ).toUInt();
			if( sr >= 44100 && sr <= 192000 )
			{
				os.samplerate = sr;
			}
			else
			{
				printf( "\nInvalid samplerate %s.\n\n"
	"Try \"%s --help\" for more information.\n\n", argv[i + 1], argv[0] );
				return( EXIT_FAILURE );
			}
			++i;
		}
		else if( argc > i &&
				( QString( argv[i] ) == "--bitrate" ||
						QString( argv[i] ) == "-b" ) )
		{
			int br = QString( argv[i + 1] ).toUInt();
			if( br >= 64 && br <= 384 )
			{
				os.bitrate = br;
			}
			else
			{
				printf( "\nInvalid bitrate %s.\n\n"
	"Try \"%s --help\" for more information.\n\n", argv[i + 1], argv[0] );
				return( EXIT_FAILURE );
			}
			++i;
		}
		else if( argc > i &&
				( QString( argv[i] ) == "--interpolation" ||
						QString( argv[i] ) == "-i" ) )
		{
			const QString ip = QString( argv[i + 1] );
			if( ip == "linear" )
			{
		qs.interpolation = mixer::qualitySettings::Interpolation_Linear;
			}
			else if( ip == "sincfastest" )
			{
		qs.interpolation = mixer::qualitySettings::Interpolation_SincFastest;
			}
			else if( ip == "sincmedium" )
			{
		qs.interpolation = mixer::qualitySettings::Interpolation_SincMedium;
			}
			else if( ip == "sincbest" )
			{
		qs.interpolation = mixer::qualitySettings::Interpolation_SincBest;
			}
			else
			{
				printf( "\nInvalid interpolation method %s.\n\n"
	"Try \"%s --help\" for more information.\n\n", argv[i + 1], argv[0] );
				return( EXIT_FAILURE );
			}
			++i;
		}
		else if( argc > i &&
				( QString( argv[i] ) == "--oversampling" ||
						QString( argv[i] ) == "-x" ) )
		{
			int os = QString( argv[i + 1] ).toUInt();
			switch( os )
			{
				case 1:
		qs.oversampling = mixer::qualitySettings::Oversampling_None;
		break;
				case 2:
		qs.oversampling = mixer::qualitySettings::Oversampling_2x;
		break;
				case 4:
		qs.oversampling = mixer::qualitySettings::Oversampling_4x;
		break;
				case 8:
		qs.oversampling = mixer::qualitySettings::Oversampling_8x;
		break;
				default:
				printf( "\nInvalid oversampling %s.\n\n"
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


	QString pos = QLocale::system().name().left( 2 );
	// load translation for Qt-widgets/-dialogs
	loadTranslation( QString( "qt_" ) + pos,
					QString( LMMS_QT_TRANSLATIONS_DIR ) );
	// load actual translation for LMMS
	loadTranslation( pos );

//	app.setFont( pointSize<10>( app.font() ) );


	if( !configManager::inst()->loadConfigFile() )
	{
		return( EXIT_FAILURE );
	}

	if( render_out == "" )
	{
		QApplication::setStyle( new lmmsStyle() );

		// set palette
		QPalette pal = qApp->palette();
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
		qApp->setPalette( pal );


		// init splash screen - this is a bit difficult as we have a
		// semi-transparent splash-image therefore we first need to grab
		// the screen, paint the splash onto it and then set a mask
		// which covers all pixels which are not fully transparent in
		// splash-image - otherwise we get nasty edges etc.
		const QPixmap splash = embed::getIconPixmap( "splash" );
    		const QPoint pt = QApplication::desktop()->
			availableGeometry().center() - splash.rect().center();
		QPixmap pm = QPixmap::grabWindow(
					QApplication::desktop()->winId(),
					pt.x(), pt.y(),
					splash.width(), splash.height() );
		QPainter p( &pm );
		p.drawPixmap( 0, 0, splash );
		p.end();

		QSplashScreen * ss = new QSplashScreen( pm );
		ss->setMask( splash.alphaChannel().createMaskFromColor(
							QColor( 0, 0, 0 ) ) );
		ss->show();
		qApp->processEvents();

		// init central engine which handles all components of LMMS
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

		engine::getMainWindow()->showMaximized();

		delete ss;
	}
	else
	{
		// we're going to render our song
		engine::init( FALSE );
		engine::getSong()->loadProject( file_to_load );

		// create renderer
		projectRenderer * r = new projectRenderer( qs, os, eff,
			render_out +
				QString( ( eff == projectRenderer::WaveFile ) ?
							"wav" : "ogg" ) );
		QCoreApplication::instance()->connect( r, SIGNAL( finished() ),
							SLOT( quit() ) );

		// timer for progress-updates
		QTimer * t = new QTimer( r );
		r->connect( t, SIGNAL( timeout() ),
				SLOT( updateConsoleProgress() ) );
		t->start( 50 );

		// start now!
		r->startProcessing();
	}

	const int ret = app->exec();
	delete app;
	return( ret );
}


#endif
