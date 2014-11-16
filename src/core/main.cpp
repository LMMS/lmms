/*
 * main.cpp - just main.cpp which is starting up app...
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2012-2013 Paul Giblock    <p/at/pgiblock.net>
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

#include "lmmsconfig.h"
#include "lmmsversion.h"
#include "versioninfo.h"

// denormals stripping
#ifdef __SSE__
#include <xmmintrin.h>
#endif
#ifdef __SSE3__
#include <pmmintrin.h>
#endif

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QLocale>
#include <QtCore/QProcess>
#include <QtCore/QTimer>
#include <QtCore/QTranslator>
#include <QtGui/QApplication>
#include <QtGui/QBitmap>
#include <QtGui/QDesktopWidget>
#include <QtGui/QMessageBox>
#include <QtGui/QPainter>
#include <QtGui/QSplashScreen>

#ifdef LMMS_HAVE_SCHED_H
#include <sched.h>
#endif

#ifdef LMMS_HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef LMMS_HAVE_PROCESS_H
#include <process.h>
#endif

#ifdef LMMS_HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "config_mgr.h"
#include "embed.h"
#include "engine.h"
#include "LmmsStyle.h"
#include "ImportFilter.h"
#include "MainWindow.h"
#include "ProjectRenderer.h"
#include "DataFile.h"
#include "song.h"
#include "LmmsPalette.h"

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
	// intialize RNG
	srand( getpid() + time( 0 ) );

	// set denormal protection for this thread
	#ifdef __SSE3__
	/* DAZ flag */
	_MM_SET_DENORMALS_ZERO_MODE( _MM_DENORMALS_ZERO_ON );
	#endif
	#ifdef __SSE__
	/* FTZ flag */
	_MM_SET_FLUSH_ZERO_MODE( _MM_FLUSH_ZERO_ON );
	#endif

	bool core_only = false;
	bool fullscreen = true;
	bool exit_after_import = false;
	QString file_to_load, file_to_save, file_to_import, render_out, profilerOutputFile;

	for( int i = 1; i < argc; ++i )
	{
		if( argc > i && ( ( QString( argv[i] ) == "--render" ||
					QString( argv[i] ) == "-r" ) ||
				( QString( argv[i] ) == "--help" ||
						QString( argv[i] ) == "-h" ) ) )
		{
			core_only = true;
		}
		else if( argc > i && QString( argv[i] ) == "-geometry" )
		{
			// option -geometry is filtered by Qt later,
			// so we need to check its presence now to
			// determine, if the application should run in
			// fullscreen mode (default, no -geometry given).
			fullscreen = false;
		}
	}

	QCoreApplication * app = core_only ?
			new QCoreApplication( argc, argv ) :
					new QApplication( argc, argv ) ;

	Mixer::qualitySettings qs( Mixer::qualitySettings::Mode_HighQuality );
	ProjectRenderer::OutputSettings os( 44100, false, 160,
						ProjectRenderer::Depth_16Bit );
	ProjectRenderer::ExportFileFormats eff = ProjectRenderer::WaveFile;


	for( int i = 1; i < argc; ++i )
	{
		if( QString( argv[i] ) == "--version" ||
						QString( argv[i] ) == "-v" )
		{
			printf( "LMMS %s\n(%s %s, Qt %s, %s)\n\n"
	"Copyright (c) 2004-2014 LMMS developers.\n\n"
	"This program is free software; you can redistribute it and/or\n"
	"modify it under the terms of the GNU General Public\n"
	"License as published by the Free Software Foundation; either\n"
	"version 2 of the License, or (at your option) any later version.\n\n"
	"Try \"%s --help\" for more information.\n\n", LMMS_VERSION,
				PLATFORM, MACHINE, QT_VERSION_STR, GCC_VERSION,
				argv[0] );

			return( EXIT_SUCCESS );
		}
		else if( argc > i && ( QString( argv[i] ) == "--help" ||
						QString( argv[i] ) == "-h" ) )
		{
			printf( "LMMS %s\n"
	"Copyright (c) 2004-2014 LMMS developers.\n\n"
	"usage: lmms [ -r <project file> ] [ options ]\n"
	"            [ -u <in> <out> ]\n"
	"            [ -d <in> ]\n"
	"            [ -h ]\n"
	"            [ <file to load> ]\n\n"
	"-r, --render <project file>	render given project file\n"
	"-o, --output <file>		render into <file>\n"
	"-f, --output-format <format>	specify format of render-output where\n"
	"				format is either 'wav' or 'ogg'.\n"
	"-s, --samplerate <samplerate>	specify output samplerate in Hz\n"
	"				range: 44100 (default) to 192000\n"
	"-b, --bitrate <bitrate>		specify output bitrate in kHz\n"
	"				default: 160.\n"
	"-i, --interpolation <method>	specify interpolation method\n"
	"				possible values:\n"
	"				   - linear\n"
	"				   - sincfastest (default)\n"
	"				   - sincmedium\n"
	"				   - sincbest\n"
	"-x, --oversampling <value>	specify oversampling\n"
	"				possible values: 1, 2, 4, 8\n"
	"				default: 2\n"
	"-u, --upgrade <in> [out]	upgrade file <in> and save as <out>\n"
	"       standard out is used if no output file is specifed\n"
	"-d, --dump <in>			dump XML of compressed file <in>\n"
	"-v, --version			show version information and exit.\n"
	"-h, --help			show this usage information and exit.\n\n",
							LMMS_VERSION );
			return( EXIT_SUCCESS );
		}
		else if( argc > i+1 && ( QString( argv[i] ) == "--upgrade" ||
						QString( argv[i] ) == "-u" ) )
		{
			QString inFile( argv[i + 1] );
			DataFile dataFile( inFile );
			if (argc > i+2)
			{
				const QString outFile = argv[i + 2];
				dataFile.writeFile( outFile );
			}
			else
			{
				QTextStream ts( stdout );
				dataFile.write( ts );
				fflush( stdout );
			}
			return( EXIT_SUCCESS );
		}
		else if( argc > i && ( QString( argv[i] ) == "--dump" ||
						QString( argv[i] ) == "-d" ) )
		{
			QFile f( argv[i + 1] );
			f.open( QIODevice::ReadOnly );
			QString d = qUncompress( f.readAll() );
			printf( "%s\n", d.toUtf8().constData() );
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
				eff = ProjectRenderer::WaveFile;
			}
#ifdef LMMS_HAVE_OGGVORBIS
			else if( ext == "ogg" )
			{
				eff = ProjectRenderer::OggFile;
			}
#endif
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
		qs.interpolation = Mixer::qualitySettings::Interpolation_Linear;
			}
			else if( ip == "sincfastest" )
			{
		qs.interpolation = Mixer::qualitySettings::Interpolation_SincFastest;
			}
			else if( ip == "sincmedium" )
			{
		qs.interpolation = Mixer::qualitySettings::Interpolation_SincMedium;
			}
			else if( ip == "sincbest" )
			{
		qs.interpolation = Mixer::qualitySettings::Interpolation_SincBest;
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
			int o = QString( argv[i + 1] ).toUInt();
			switch( o )
			{
				case 1:
		qs.oversampling = Mixer::qualitySettings::Oversampling_None;
		break;
				case 2:
		qs.oversampling = Mixer::qualitySettings::Oversampling_2x;
		break;
				case 4:
		qs.oversampling = Mixer::qualitySettings::Oversampling_4x;
		break;
				case 8:
		qs.oversampling = Mixer::qualitySettings::Oversampling_8x;
		break;
				default:
				printf( "\nInvalid oversampling %s.\n\n"
	"Try \"%s --help\" for more information.\n\n", argv[i + 1], argv[0] );
				return( EXIT_FAILURE );
			}
			++i;
		}
		else if( argc > i &&
				( QString( argv[i] ) == "--import" ) )
		{
			file_to_import = argv[i+1];
			++i;
			// exit after import? (only for debugging)
			if( argc > i && QString( argv[i+1] ) == "-e" )
			{
				exit_after_import = true;
			}
		}
		else if( argc > i && ( QString( argv[i] ) == "--profile" || QString( argv[i] ) == "-p" ) )
		{
			profilerOutputFile = argv[i+1];
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

#ifdef LMMS_BUILD_WIN32
#undef QT_TRANSLATIONS_DIR
#define QT_TRANSLATIONS_DIR configManager::inst()->localeDir()
#endif

#ifdef QT_TRANSLATIONS_DIR
	// load translation for Qt-widgets/-dialogs
	loadTranslation( QString( "qt_" ) + pos,
					QString( QT_TRANSLATIONS_DIR ) );
#endif
	// load actual translation for LMMS
	loadTranslation( pos );


	// try to set realtime priority
#ifdef LMMS_BUILD_LINUX
#ifdef LMMS_HAVE_SCHED_H
#ifndef __OpenBSD__
	struct sched_param sparam;
	sparam.sched_priority = ( sched_get_priority_max( SCHED_FIFO ) +
				sched_get_priority_min( SCHED_FIFO ) ) / 2;
	if( sched_setscheduler( 0, SCHED_FIFO, &sparam ) == -1 )
	{
		printf( "Notice: could not set realtime priority.\n" );
	}
#endif
#endif
#endif

	configManager::inst()->loadConfigFile();

	if( render_out.isEmpty() )
	{
		// init style and palette
		LmmsStyle * lmmsstyle = new LmmsStyle();
		QApplication::setStyle( lmmsstyle );

		LmmsPalette * lmmspal = new LmmsPalette( NULL, lmmsstyle );
		QPalette lpal = lmmspal->palette();

		QApplication::setPalette( lpal );
		LmmsStyle::s_palette = &lpal;


		// show splash screen
		QSplashScreen splashScreen( embed::getIconPixmap( "splash" ) );
		splashScreen.show();
		splashScreen.showMessage( MainWindow::tr( "Version %1" ).arg( LMMS_VERSION ),
									Qt::AlignRight | Qt::AlignBottom, Qt::white );
		qApp->processEvents();

		// init central engine which handles all components of LMMS
		engine::init();

		splashScreen.hide();

		// re-intialize RNG - shared libraries might have srand() or
		// srandom() calls in their init procedure
		srand( getpid() + time( 0 ) );

		// recover a file?
		QString recoveryFile = QDir(configManager::inst()->workingDir()).absoluteFilePath("recover.mmp");
		if( QFileInfo(recoveryFile).exists() &&
			QMessageBox::question( engine::mainWindow(), MainWindow::tr( "Project recovery" ),
						MainWindow::tr( "It looks like the last session did not end properly. "
										"Do you want to recover the project of this session?" ),
						QMessageBox::Yes | QMessageBox::No ) == QMessageBox::Yes )
		{
			file_to_load = recoveryFile;
		}

		// we try to load given file
		if( !file_to_load.isEmpty() )
		{
			engine::mainWindow()->show();
			if( fullscreen )
			{
				engine::mainWindow()->showMaximized();
			}
			if( file_to_load == recoveryFile )
			{
				engine::getSong()->createNewProjectFromTemplate( file_to_load );
			}
			else
			{
				engine::getSong()->loadProject( file_to_load );
			}
		}
		else if( !file_to_import.isEmpty() )
		{
			ImportFilter::import( file_to_import, engine::getSong() );
			if( exit_after_import )
			{
				return 0;
			}

			engine::mainWindow()->show();
			if( fullscreen )
			{
				engine::mainWindow()->showMaximized();
			}
		}
		else
		{
			engine::getSong()->createNewProject();

			// [Settel] workaround: showMaximized() doesn't work with
			// FVWM2 unless the window is already visible -> show() first
			engine::mainWindow()->show();
			if( fullscreen )
			{
				engine::mainWindow()->showMaximized();
			}
		}
	}
	else
	{
		// we're going to render our song
		engine::init( false );
		printf( "loading project...\n" );
		engine::getSong()->loadProject( file_to_load );
		printf( "done\n" );

		// create renderer
		ProjectRenderer * r = new ProjectRenderer( qs, os, eff,
			render_out +
				QString( ( eff ==
					ProjectRenderer::WaveFile ) ?
						"wav" : "ogg" ) );
		QCoreApplication::instance()->connect( r,
				SIGNAL( finished() ), SLOT( quit() ) );

		// timer for progress-updates
		QTimer * t = new QTimer( r );
		r->connect( t, SIGNAL( timeout() ),
				SLOT( updateConsoleProgress() ) );
		t->start( 200 );

		if( profilerOutputFile.isEmpty() == false )
		{
			engine::mixer()->profiler().setOutputFile( profilerOutputFile );
		}

		// start now!
		r->startProcessing();
	}

	const int ret = app->exec();
	delete app;
	return( ret );
}


/* vim: set tw=0 noexpandtab: */
