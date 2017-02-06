/*
 * main.cpp - just main.cpp which is starting up app...
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2012-2013 Paul Giblock    <p/at/pgiblock.net>
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

#include "lmmsconfig.h"
#include "lmmsversion.h"
#include "versioninfo.h"

#include "denormals.h"

#include <QFileInfo>
#include <QLocale>
#include <QDate>
#include <QTimer>
#include <QTranslator>
#include <QApplication>
#include <QMessageBox>
#include <QPushButton>
#include <QTextStream>

#ifdef LMMS_BUILD_WIN32
#include <windows.h>
#endif

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

#include <signal.h>

#include "MemoryManager.h"
#include "ConfigManager.h"
#include "NotePlayHandle.h"
#include "embed.h"
#include "Engine.h"
#include "GuiApplication.h"
#include "ImportFilter.h"
#include "MainWindow.h"
#include "ProjectRenderer.h"
#include "RenderManager.h"
#include "DataFile.h"
#include "Song.h"
#include "SetupDialog.h"

static inline QString baseName( const QString & file )
{
	return QFileInfo( file ).absolutePath() + "/" +
			QFileInfo( file ).completeBaseName();
}




inline void loadTranslation( const QString & tname,
	const QString & dir = ConfigManager::inst()->localeDir() )
{
	QTranslator * t = new QTranslator( QCoreApplication::instance() );
	QString name = tname + ".qm";

	t->load( name, dir );

	QCoreApplication::instance()->installTranslator( t );
}




void printVersion( char *executableName )
{
	printf( "LMMS %s\n(%s %s, Qt %s, %s)\n\n"
		"Copyright (c) %s\n\n"
		"This program is free software; you can redistribute it and/or\n"
		"modify it under the terms of the GNU General Public\n"
		"License as published by the Free Software Foundation; either\n"
		"version 2 of the License, or (at your option) any later version.\n\n"
		"Try \"%s --help\" for more information.\n\n", LMMS_VERSION,
		PLATFORM, MACHINE, QT_VERSION_STR, GCC_VERSION,
		LMMS_PROJECT_COPYRIGHT, executableName );
}




void printHelp()
{
	printf( "LMMS %s\n"
		"Copyright (c) %s\n\n"
		"Usage: lmms [ -a ]\n"
		"            [ -b <bitrate> ]\n"
		"            [ -c <configfile> ]\n"
		"            [ -d <in> ]\n"
		"            [ -f <format> ]\n"
		"            [ --geometry <geometry> ]\n"
		"            [ -h ]\n"
		"            [ -i <method> ]\n"
		"            [ --import <in> [-e]]\n"
		"            [ -l ]\n"
		"            [ -o <path> ]\n"
		"            [ -p <out> ]\n"
		"            [ -r <project file> ] [ options ]\n"
		"            [ -s <samplerate> ]\n"
		"            [ -u <in> <out> ]\n"
		"            [ -v ]\n"
		"            [ -x <value> ]\n"
		"            [ <file to load> ]\n\n"
		"-a, --float                   32bit float bit depth\n"
		"-b, --bitrate <bitrate>       Specify output bitrate in KBit/s\n"
		"       Default: 160.\n"
		"-c, --config <configfile>     Get the configuration from <configfile>\n"
		"-d, --dump <in>               Dump XML of compressed file <in>\n"
		"-f, --format <format>         Specify format of render-output where\n"
		"       Format is either 'wav' or 'ogg'.\n"
		"    --geometry <geometry>     Specify the size and position of the main window\n"
		"       geometry is <xsizexysize+xoffset+yoffsety>.\n"
		"-h, --help                    Show this usage information and exit.\n"
		"-i, --interpolation <method>  Specify interpolation method\n"
		"       Possible values:\n"
		"          - linear\n"
		"          - sincfastest (default)\n"
		"          - sincmedium\n"
		"          - sincbest\n"
		"    --import <in> [-e]        Import MIDI file <in>.\n"
		"       If -e is specified lmms exits after importing the file.\n"
		"-l, --loop                    Render as a loop\n"
		"-o, --output <path>           Render into <path>\n"
		"       For --render, provide a file path\n"
		"       For --rendertracks, provide a directory path\n"
		"-p, --profile <out>           Dump profiling information to file <out>\n"
		"-r, --render <project file>   Render given project file\n"
		"    --rendertracks <project>  Render each track to a different file\n"
		"-s, --samplerate <samplerate> Specify output samplerate in Hz\n"
		"       Range: 44100 (default) to 192000\n"
		"-u, --upgrade <in> [out]      Upgrade file <in> and save as <out>\n"
		"       Standard out is used if no output file is specifed\n"
		"-v, --version                 Show version information and exit.\n"
		"    --allowroot               Bypass root user startup check (use with caution).\n"
		"-x, --oversampling <value>    Specify oversampling\n"
		"       Possible values: 1, 2, 4, 8\n"
		"       Default: 2\n\n",
		LMMS_VERSION, LMMS_PROJECT_COPYRIGHT );
}




void fileCheck( QString &file )
{
	QFileInfo fileToCheck( file );

	if( !fileToCheck.size() )
	{
		printf( "The file %s does not have any content.\n",
				file.toUtf8().constData() );
		exit( EXIT_FAILURE );
	}
	else if( fileToCheck.isDir() )
	{
		printf( "%s is a directory.\n",
				file.toUtf8().constData() );
		exit( EXIT_FAILURE );
	}
}




int main( int argc, char * * argv )
{
	// initialize memory managers
	MemoryManager::init();
	NotePlayHandleManager::init();

	// intialize RNG
	srand( getpid() + time( 0 ) );

	disable_denormals();

	bool coreOnly = false;
	bool fullscreen = true;
	bool exitAfterImport = false;
	bool allowRoot = false;
	bool renderLoop = false;
	bool renderTracks = false;
	QString fileToLoad, fileToImport, renderOut, profilerOutputFile, configFile;

	// first of two command-line parsing stages
	for( int i = 1; i < argc; ++i )
	{
		QString arg = argv[i];

		if( arg == "--help"    || arg == "-h" ||
		    arg == "--version" || arg == "-v" ||
		    arg == "--render"  || arg == "-r" )
		{
			coreOnly = true;
		}
		else if( arg == "--rendertracks" )
		{
			coreOnly = true;
			renderTracks = true;
		}
		else if( arg == "--allowroot" )
		{
			allowRoot = true;
		}
		else if( arg == "--geometry" || arg == "-geometry")
		{
			if( arg == "--geometry" )
			{
				// Delete the first "-" so Qt recognize the option
				strcpy(argv[i], "-geometry");
			}
			// option -geometry is filtered by Qt later,
			// so we need to check its presence now to
			// determine, if the application should run in
			// fullscreen mode (default, no -geometry given).
			fullscreen = false;
		}
	}

#ifndef LMMS_BUILD_WIN32
	if ( ( getuid() == 0 || geteuid() == 0 ) && !allowRoot )
	{
		printf( "LMMS cannot be run as root.\nUse \"--allowroot\" to override.\n\n" );
		return EXIT_FAILURE;
	}	
#endif

	QCoreApplication * app = coreOnly ?
			new QCoreApplication( argc, argv ) :
					new QApplication( argc, argv ) ;

	Mixer::qualitySettings qs( Mixer::qualitySettings::Mode_HighQuality );
	ProjectRenderer::OutputSettings os( 44100, false, 160,
						ProjectRenderer::Depth_16Bit );
	ProjectRenderer::ExportFileFormats eff = ProjectRenderer::WaveFile;

	// second of two command-line parsing stages
	for( int i = 1; i < argc; ++i )
	{
		QString arg = argv[i];

		if( arg == "--version" || arg == "-v" )
		{
			printVersion( argv[0] );
			return EXIT_SUCCESS;
		}
		else if( arg == "--help" || arg  == "-h" )
		{
			printHelp();
			return EXIT_SUCCESS;
		}
		else if( arg == "--upgrade" || arg == "-u" )
		{
			++i;

			if( i == argc )
			{
				printf( "\nNo input file specified.\n\n"
	"Try \"%s --help\" for more information.\n\n", argv[0] );
				return EXIT_FAILURE;
			}


			DataFile dataFile( QString::fromLocal8Bit( argv[i] ) );

			if( argc > i+1 ) // output file specified
			{
				dataFile.writeFile( QString::fromLocal8Bit( argv[i+1] ) );
			}
			else // no output file specified; use stdout
			{
				QTextStream ts( stdout );
				dataFile.write( ts );
				fflush( stdout );
			}

			return EXIT_SUCCESS;
		}
		else if( arg == "--allowroot" )
		{
			// Ignore, processed earlier
#ifdef LMMS_BUILD_WIN32
			if( allowRoot )
			{
				printf( "\nOption \"--allowroot\" will be ignored on this platform.\n\n" );
			}
#endif
			
		}
		else if( arg == "--dump" || arg == "-d" )
		{
			++i;

			if( i == argc )
			{
				printf( "\nNo input file specified.\n\n"
	"Try \"%s --help\" for more information.\n\n", argv[0] );
				return EXIT_FAILURE;
			}


			QFile f( QString::fromLocal8Bit( argv[i] ) );
			f.open( QIODevice::ReadOnly );
			QString d = qUncompress( f.readAll() );
			printf( "%s\n", d.toUtf8().constData() );

			return EXIT_SUCCESS;
		}
		else if( arg == "--render" || arg == "-r" || arg == "--rendertracks" )
		{
			++i;

			if( i == argc )
			{
				printf( "\nNo input file specified.\n\n"
	"Try \"%s --help\" for more information.\n\n", argv[0] );
				return EXIT_FAILURE;
			}


			fileToLoad = QString::fromLocal8Bit( argv[i] );
			renderOut = fileToLoad;
		}
		else if( arg == "--loop" || arg == "-l" )
		{
			renderLoop = true;
		}
		else if( arg == "--output" || arg == "-o" )
		{
			++i;

			if( i == argc )
			{
				printf( "\nNo output file specified.\n\n"
	"Try \"%s --help\" for more information.\n\n", argv[0] );
				return EXIT_FAILURE;
			}


			renderOut = QString::fromLocal8Bit( argv[i] );
		}
		else if( arg == "--format" || arg == "-f" )
		{
			++i;

			if( i == argc )
			{
				printf( "\nNo output format specified.\n\n"
	"Try \"%s --help\" for more information.\n\n", argv[0] );
				return EXIT_FAILURE;
			}


			const QString ext = QString( argv[i] );

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
	"Try \"%s --help\" for more information.\n\n", argv[i], argv[0] );
				return EXIT_FAILURE;
			}
		}
		else if( arg == "--samplerate" || arg == "-s" )
		{
			++i;

			if( i == argc )
			{
				printf( "\nNo samplerate specified.\n\n"
	"Try \"%s --help\" for more information.\n\n", argv[0] );
				return EXIT_FAILURE;
			}


			sample_rate_t sr = QString( argv[i] ).toUInt();
			if( sr >= 44100 && sr <= 192000 )
			{
				os.samplerate = sr;
			}
			else
			{
				printf( "\nInvalid samplerate %s.\n\n"
	"Try \"%s --help\" for more information.\n\n", argv[i], argv[0] );
				return EXIT_FAILURE;
			}
		}
		else if( arg == "--bitrate" || arg == "-b" )
		{
			++i;

			if( i == argc )
			{
				printf( "\nNo bitrate specified.\n\n"
	"Try \"%s --help\" for more information.\n\n", argv[0] );
				return EXIT_FAILURE;
			}


			int br = QString( argv[i] ).toUInt();

			if( br >= 64 && br <= 384 )
			{
				os.bitrate = br;
			}
			else
			{
				printf( "\nInvalid bitrate %s.\n\n"
	"Try \"%s --help\" for more information.\n\n", argv[i], argv[0] );
				return EXIT_FAILURE;
			}
		}
		else if( arg =="--float" || arg == "-a" )
		{
			os.depth = ProjectRenderer::Depth_32Bit;
		}
		else if( arg == "--interpolation" || arg == "-i" )
		{
			++i;

			if( i == argc )
			{
				printf( "\nNo interpolation method specified.\n\n"
	"Try \"%s --help\" for more information.\n\n", argv[0] );
				return EXIT_FAILURE;
			}


			const QString ip = QString( argv[i] );

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
	"Try \"%s --help\" for more information.\n\n", argv[i], argv[0] );
				return EXIT_FAILURE;
			}
		}
		else if( arg == "--oversampling" || arg == "-x" )
		{
			++i;

			if( i == argc )
			{
				printf( "\nNo oversampling specified.\n\n"
	"Try \"%s --help\" for more information.\n\n", argv[0] );
				return EXIT_FAILURE;
			}


			int o = QString( argv[i] ).toUInt();

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
	"Try \"%s --help\" for more information.\n\n", argv[i], argv[0] );
				return EXIT_FAILURE;
			}
		}
		else if( arg == "--import" )
		{
			++i;

			if( i == argc )
			{
				printf( "\nNo file specified for importing.\n\n"
	"Try \"%s --help\" for more information.\n\n", argv[0] );
				return EXIT_FAILURE;
			}


			fileToImport = QString::fromLocal8Bit( argv[i] );

			// exit after import? (only for debugging)
			if( QString( argv[i + 1] ) == "-e" )
			{
				exitAfterImport = true;
				++i;
			}
		}
		else if( arg == "--profile" || arg == "-p" )
		{
			++i;

			if( i == argc )
			{
				printf( "\nNo profile specified.\n\n"
	"Try \"%s --help\" for more information.\n\n", argv[0] );
				return EXIT_FAILURE;
			}


			profilerOutputFile = QString::fromLocal8Bit( argv[i] );
		}
		else if( arg == "--config" || arg == "-c" )
		{
			++i;

			if( i == argc )
			{
				printf( "\nNo configuration file specified.\n\n"
	"Try \"%s --help\" for more information.\n\n", argv[0] );
				return EXIT_FAILURE;
			}

			configFile = QString::fromLocal8Bit( argv[i] );
		}
		else
		{
			if( argv[i][0] == '-' )
			{
				printf( "\nInvalid option %s.\n\n"
	"Try \"%s --help\" for more information.\n\n", argv[i], argv[0] );
				return EXIT_FAILURE;
			}
			fileToLoad = QString::fromLocal8Bit( argv[i] );
		}
	}

	// Test file argument before continuing
	if( !fileToLoad.isEmpty() )
	{
		fileCheck( fileToLoad );
	}
	else if( !fileToImport.isEmpty() )
	{
		fileCheck( fileToImport );
	}

	ConfigManager::inst()->loadConfigFile(configFile);

	// set language
	QString pos = ConfigManager::inst()->value( "app", "language" );
	if( pos.isEmpty() )
	{
		pos = QLocale::system().name().left( 2 );
	}

#ifdef LMMS_BUILD_WIN32
#undef QT_TRANSLATIONS_DIR
#define QT_TRANSLATIONS_DIR ConfigManager::inst()->localeDir()
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

#ifdef LMMS_BUILD_WIN32
	if( !SetPriorityClass( GetCurrentProcess(), HIGH_PRIORITY_CLASS ) )
	{
		printf( "Notice: could not set high priority.\n" );
	}
#endif

#if _POSIX_C_SOURCE >= 1 || _XOPEN_SOURCE || _POSIX_SOURCE
	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sa.sa_flags = SA_SIGINFO;
	if ( sigemptyset( &sa.sa_mask ) )
	{
		fprintf( stderr, "Signal initialization failed.\n" );
	}
	if ( sigaction( SIGPIPE, &sa, NULL ) )
	{
		fprintf( stderr, "Signal initialization failed.\n" );
	}
#endif

	bool destroyEngine = false;

	// if we have an output file for rendering, just render the song
	// without starting the GUI
	if( !renderOut.isEmpty() )
	{
		Engine::init( true );
		destroyEngine = true;

		printf( "Loading project...\n" );
		Engine::getSong()->loadProject( fileToLoad );
		if( Engine::getSong()->isEmpty() )
		{
			printf("The project %s is empty, aborting!\n", fileToLoad.toUtf8().constData() );
			exit( EXIT_FAILURE );
		}
		printf( "Done\n" );

		Engine::getSong()->setExportLoop( renderLoop );

		// when rendering multiple tracks, renderOut is a directory
		// otherwise, it is a file, so we need to append the file extension
		if ( !renderTracks )
		{
			renderOut = baseName( renderOut ) +
				ProjectRenderer::getFileExtensionFromFormat(eff);
		}

		// create renderer
		RenderManager * r = new RenderManager( qs, os, eff, renderOut );
		QCoreApplication::instance()->connect( r,
				SIGNAL( finished() ), SLOT( quit() ) );

		// timer for progress-updates
		QTimer * t = new QTimer( r );
		r->connect( t, SIGNAL( timeout() ),
				SLOT( updateConsoleProgress() ) );
		t->start( 200 );

		if( profilerOutputFile.isEmpty() == false )
		{
			Engine::mixer()->profiler().setOutputFile( profilerOutputFile );
		}

		// start now!
		if ( renderTracks )
		{
			r->renderTracks();
		}
		else
		{
			r->renderProject();
		}
	}
	else // otherwise, start the GUI
	{
		new GuiApplication();

		// re-intialize RNG - shared libraries might have srand() or
		// srandom() calls in their init procedure
		srand( getpid() + time( 0 ) );

		// recover a file?
		QString recoveryFile = ConfigManager::inst()->recoveryFile();

		bool recoveryFilePresent = QFileInfo( recoveryFile ).exists() &&
				QFileInfo( recoveryFile ).isFile();
		bool autoSaveEnabled =
			ConfigManager::inst()->value( "ui", "enableautosave" ).toInt();
		if( recoveryFilePresent )
		{
			QMessageBox mb;
			mb.setWindowTitle( MainWindow::tr( "Project recovery" ) );
			mb.setText( QString(
				"<html>"
				"<p style=\"margin-left:6\">%1</p>"
				"<table cellpadding=\"3\">"
				"  <tr>"
				"    <td><b>%2</b></td>"
				"    <td>%3</td>"
				"  </tr>"
				"  <tr>"
				"    <td><b>%4</b></td>"
				"    <td>%5</td>"
				"  </tr>"
				"  <tr>"
				"    <td><b>%6</b></td>"
				"    <td>%7</td>"
				"  </tr>"
				"</table>"
				"</html>" ).arg(
				MainWindow::tr( "There is a recovery file present. "
					"It looks like the last session did not end "
					"properly or another instance of LMMS is "
					"already running. Do you want to recover the "
					"project of this session?" ),
				MainWindow::tr( "Recover" ),
				MainWindow::tr( "Recover the file. Please don't run "
					"multiple instances of LMMS when you do this." ),
				MainWindow::tr( "Ignore" ),
				MainWindow::tr( "Launch LMMS as usual but with "
					"automatic backup disabled to prevent the "
					"present recover file from being overwritten." ),
				MainWindow::tr( "Discard" ),
				MainWindow::tr( "Launch a default session and delete "
					"the restored files. This is not reversible." )
							) );

			mb.setIcon( QMessageBox::Warning );
			mb.setWindowIcon( embed::getIconPixmap( "icon" ) );
			mb.setWindowFlags( Qt::WindowCloseButtonHint );

			QPushButton * recover;
			QPushButton * discard;
			QPushButton * ignore;
			QPushButton * exit;
			
			#if QT_VERSION >= 0x050000
				// setting all buttons to the same roles allows us 
				// to have a custom layout
				discard = mb.addButton( MainWindow::tr( "Discard" ),
									QMessageBox::AcceptRole );
				ignore = mb.addButton( MainWindow::tr( "Ignore" ),
									QMessageBox::AcceptRole );
				recover = mb.addButton( MainWindow::tr( "Recover" ),
									QMessageBox::AcceptRole );

			# else 
				// in qt4 the button order is reversed
				recover = mb.addButton( MainWindow::tr( "Recover" ),
									QMessageBox::AcceptRole );
				ignore = mb.addButton( MainWindow::tr( "Ignore" ),
									QMessageBox::AcceptRole );
				discard = mb.addButton( MainWindow::tr( "Discard" ),
									QMessageBox::AcceptRole );

			#endif
			
			// have a hidden exit button
			exit = mb.addButton( "", QMessageBox::RejectRole);
			exit->setVisible(false);
			
			// set icons
			recover->setIcon( embed::getIconPixmap( "recover" ) );
			discard->setIcon( embed::getIconPixmap( "discard" ) );
			ignore->setIcon( embed::getIconPixmap( "ignore" ) );

			mb.setDefaultButton( recover );
			mb.setEscapeButton( exit );

			mb.exec();
			if( mb.clickedButton() == discard )
			{
				gui->mainWindow()->sessionCleanup();
			}
			else if( mb.clickedButton() == recover ) // Recover
			{
				fileToLoad = recoveryFile;
				gui->mainWindow()->setSession( MainWindow::SessionState::Recover );
			}
			else if( mb.clickedButton() == ignore )
			{
				if( autoSaveEnabled )
				{
					gui->mainWindow()->setSession( MainWindow::SessionState::Limited );
				}
			}
			else // Exit
			{
				return 0;
			}
		}

		// first show the Main Window and then try to load given file

		// [Settel] workaround: showMaximized() doesn't work with
		// FVWM2 unless the window is already visible -> show() first
		gui->mainWindow()->show();
		if( fullscreen )
		{
			gui->mainWindow()->showMaximized();
		}

		if( !fileToLoad.isEmpty() )
		{
			if( fileToLoad == recoveryFile )
			{
				Engine::getSong()->createNewProjectFromTemplate( fileToLoad );
			}
			else
			{
				Engine::getSong()->loadProject( fileToLoad );
			}
		}
		else if( !fileToImport.isEmpty() )
		{
			ImportFilter::import( fileToImport, Engine::getSong() );
			if( exitAfterImport )
			{
				return EXIT_SUCCESS;
			}
		}
		// If enabled, open last project if there is one. Else, create
		// a new one. Also skip recently opened file if limited session to
		// lower the chance of opening an already opened file.
		else if( ConfigManager::inst()->
				value( "app", "openlastproject" ).toInt() &&
			!ConfigManager::inst()->
				recentlyOpenedProjects().isEmpty() &&
			gui->mainWindow()->getSession() !=
				MainWindow::SessionState::Limited )
		{
			QString f = ConfigManager::inst()->
					recentlyOpenedProjects().first();
			QFileInfo recentFile( f );

			if ( recentFile.exists() )
			{
				Engine::getSong()->loadProject( f );
			}
			else
			{
				Engine::getSong()->createNewProject();
			}
		}
		else
		{
			Engine::getSong()->createNewProject();
		}

		// Finally we start the auto save timer and also trigger the
		// autosave one time as recover.mmp is a signal to possible other
		// instances of LMMS.
		if( autoSaveEnabled &&
			gui->mainWindow()->getSession() != MainWindow::SessionState::Limited )
		{
			gui->mainWindow()->autoSaveTimerReset();
			gui->mainWindow()->autoSave();
		}
	}

	const int ret = app->exec();
	delete app;

	if( destroyEngine )
	{
		Engine::destroy();
	}

	// cleanup memory managers
	MemoryManager::cleanup();

	// ProjectRenderer::updateConsoleProgress() doesn't return line after render
	if( coreOnly )
	{
		printf( "\n" );
	}

	return ret;
}
