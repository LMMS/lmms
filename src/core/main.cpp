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

#include <QDebug>
#include <QFileInfo>
#include <QLocale>
#include <QTimer>
#include <QTranslator>
#include <QApplication>
#include <QMessageBox>
#include <QPushButton>
#include <QTextStream>

#ifdef LMMS_BUILD_WIN32
#include <windows.h>
#endif

#ifdef LMMS_HAVE_PROCESS_H
#include <process.h>
#endif

#ifdef LMMS_HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef LMMS_HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif

#include <csignal>  // To register the signal handler

#include "MainApplication.h"
#include "ConfigManager.h"
#include "DataFile.h"
#include "NotePlayHandle.h"
#include "embed.h"
#include "Engine.h"
#include "GuiApplication.h"
#include "ImportFilter.h"
#include "MainWindow.h"
#include "MixHelpers.h"
#include "OutputSettings.h"
#include "ProjectRenderer.h"
#include "RenderManager.h"
#include "Song.h"

#ifdef LMMS_DEBUG_FPE
#include <fenv.h> // For feenableexcept
#include <execinfo.h> // For backtrace and backtrace_symbols_fd
#include <unistd.h> // For STDERR_FILENO
#endif


#ifdef LMMS_DEBUG_FPE
void sigfpeHandler(int signum)
{

	// Get a back trace
	void *array[10];
	size_t size;

	// get void*'s for all entries on the stack
	size = backtrace(array, 10);

	backtrace_symbols_fd(array, size, STDERR_FILENO);

	// cleanup and close up stuff here
	// terminate program

	exit(signum);
}
#endif

static inline QString baseName( const QString & file )
{
	return QFileInfo( file ).absolutePath() + "/" +
			QFileInfo( file ).completeBaseName();
}


#ifdef LMMS_BUILD_WIN32
// Workaround for old MinGW
#ifdef __MINGW32__
extern "C" _CRTIMP errno_t __cdecl freopen_s(FILE** _File,
	const char *_Filename, const char *_Mode, FILE *_Stream);
#endif

// For qInstallMessageHandler
void consoleMessageHandler(QtMsgType type,
	const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    fprintf(stderr, "%s\n", localMsg.constData());
}
#endif // LMMS_BUILD_WIN32


inline void loadTranslation( const QString & tname,
	const QString & dir = lmms::ConfigManager::inst()->localeDir() )
{
	auto t = new QTranslator(QCoreApplication::instance());
	QString name = tname + ".qm";

	if (t->load(name, dir))
	{
		QCoreApplication::instance()->installTranslator(t);
	}
}




void printVersion( char *executableName )
{
	printf("LMMS %s\n(%s %s, Qt %s, %s)\n\n"
		"Build options:\n%s\n\n"
		"Copyright (c) %s\n\n"
		"This program is free software; you can redistribute it and/or\n"
		"modify it under the terms of the GNU General Public\n"
		"License as published by the Free Software Foundation; either\n"
		"version 2 of the License, or (at your option) any later version.\n\n"
		"Try \"%s --help\" for more information.\n\n", LMMS_VERSION,
		LMMS_BUILDCONF_PLATFORM, LMMS_BUILDCONF_MACHINE, QT_VERSION_STR, LMMS_BUILDCONF_COMPILER_VERSION, LMMS_BUILD_OPTIONS,
		LMMS_PROJECT_COPYRIGHT, executableName);
}




void printHelp()
{
	printf( "LMMS %s\n"
		"Copyright (c) %s\n\n"
		"Usage: lmms [global options...] [<action> [action parameters...]]\n\n"
		"Actions:\n"
		"  <no action> [options...] [<project>]  Start LMMS in normal GUI mode\n"
		"  dump <in>                             Dump XML of compressed file <in>\n"
		"  compress <in>                         Compress file <in>\n"
		"  render <project> [options...]         Render given project file\n"
		"  rendertracks <project> [options...]   Render each track to a different file\n"
		"  upgrade <in> [out]                    Upgrade file <in> and save as <out>\n"
		"                                        Standard out is used if no output file\n"
		"                                        is specified\n"
		"  makebundle <in> [out]                 Make a project bundle from the project\n"
		"                                        file <in> saving the resulting bundle\n"
		"                                        as <out>\n"
		"\nGlobal options:\n"
		"      --allowroot                Bypass root user startup check (use with\n"
		"          caution).\n"
		"  -c, --config <configfile>      Get the configuration from <configfile>\n"
		"  -h, --help                     Show this usage information and exit.\n"
		"  -v, --version                  Show version information and exit.\n"
		"\nOptions if no action is given:\n"
		"      --geometry <geometry>      Specify the size and position of\n"
		"          the main window\n"
		"          geometry is <xsizexysize+xoffset+yoffsety>.\n"
		"      --import <in> [-e]         Import MIDI or Hydrogen file <in>.\n"
		"          If -e is specified lmms exits after importing the file.\n"
		"\nOptions for \"render\" and \"rendertracks\":\n"
		"  -a, --float                    Use 32bit float bit depth\n"
		"  -b, --bitrate <bitrate>        Specify output bitrate in KBit/s\n"
		"          Default: 160.\n"
		"  -f, --format <format>         Specify format of render-output where\n"
		"          Format is either 'wav', 'flac', 'ogg' or 'mp3'.\n"
		"  -i, --interpolation <method>   Specify interpolation method\n"
		"          Possible values:\n"
		"            - linear\n"
		"            - sincfastest (default)\n"
		"            - sincmedium\n"
		"            - sincbest\n"
		"  -l, --loop                     Render as a loop\n"
		"  -m, --mode                     Stereo mode used for MP3 export\n"
		"          Possible values: s, j, m\n"
		"            s: Stereo\n"
		"            j: Joint Stereo\n"
		"            m: Mono\n"
		"          Default: j\n"
		"  -o, --output <path>            Render into <path>\n"
		"          For \"render\", provide a file path\n"
		"          For \"rendertracks\", provide a directory path\n"
		"          If not specified, render will overwrite the input file\n"
		"          For \"rendertracks\", this might be required\n"
		"  -p, --profile <out>            Dump profiling information to file <out>\n"
		"  -s, --samplerate <samplerate>  Specify output samplerate in Hz\n"
		"          Range: 44100 (default) to 192000\n"
		"          Possible values: 1, 2, 4, 8\n"
		"          Default: 2\n\n",
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

int usageError(const QString& message)
{
	qCritical().noquote() << QString("\n%1.\n\nTry \"%2 --help\" for more information.\n\n")
				   .arg( message ).arg( qApp->arguments()[0] );
	return EXIT_FAILURE;
}

int noInputFileError()
{
	return usageError( "No input file specified" );
}


int main( int argc, char * * argv )
{
	using namespace lmms;

	bool coreOnly = false;
	bool fullscreen = true;
	bool exitAfterImport = false;
	bool allowRoot = false;
	bool renderLoop = false;
	bool renderTracks = false;
	QString fileToLoad, fileToImport, renderOut, profilerOutputFile, configFile;

	// first of two command-line parsing stages
	for (int i = 1; i < argc; ++i)
	{
		QString arg = argv[i];

		if (arg == "--help" || arg == "-h")
		{
			printHelp();
			return EXIT_SUCCESS;
		}
		else if (arg == "--version" || arg == "-v")
		{
			printVersion(argv[0]);
			return EXIT_SUCCESS;
		}
		else if (arg == "render" || arg == "--render" || arg == "-r" )
		{
			coreOnly = true;
		}
		else if (arg == "rendertracks" || arg == "--rendertracks")
		{
			coreOnly = true;
			renderTracks = true;
		}
		else if (arg == "--allowroot")
		{
			allowRoot = true;
		}
		else if (arg == "--geometry" || arg == "-geometry")
		{
			if (arg == "--geometry")
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

#ifdef LMMS_DEBUG_FPE
	// Enable exceptions for certain floating point results
	// FE_UNDERFLOW is disabled for the time being
	feenableexcept( FE_INVALID   |
			FE_DIVBYZERO |
			FE_OVERFLOW  /*|
			FE_UNDERFLOW*/);

	// Install the trap handler
	// register signal SIGFPE and signal handler
	signal(SIGFPE, sigfpeHandler);
#endif
	signal(SIGINT, gui::GuiApplication::sigintHandler);

#ifdef LMMS_BUILD_WIN32
	// Don't touch redirected streams here
	// GetStdHandle should be called before AttachConsole
	HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE hStdErr = GetStdHandle(STD_ERROR_HANDLE);
	FILE *fIn, *fOut, *fErr;
	// Enable console output if available
	if (AttachConsole(ATTACH_PARENT_PROCESS))
	{
		if (!hStdIn)
		{
			freopen_s(&fIn, "CONIN$", "r", stdin);
		}
		if (!hStdOut)
		{
			freopen_s(&fOut, "CONOUT$", "w", stdout);
		}
		if (!hStdErr)
		{
			freopen_s(&fErr, "CONOUT$", "w", stderr);
		}
	}
	// Make Qt's debug message handlers work
	qInstallMessageHandler(consoleMessageHandler);
#endif

#if defined(LMMS_HAVE_SYS_PRCTL_H) && defined(PR_SET_CHILD_SUBREAPER)
	// Set the "child subreaper" attribute so that plugin child processes remain as lmms'
	// children even when some wrapper process exits, as it may happen with wine
	if (prctl(PR_SET_CHILD_SUBREAPER, 1))
	{
		perror("prctl(PR_SET_CHILD_SUBREAPER)");
	}
#endif

	// initialize memory managers
	NotePlayHandleManager::init();

	// intialize RNG
	srand( getpid() + time( 0 ) );

	disable_denormals();

#if !defined(LMMS_BUILD_WIN32) && !defined(LMMS_BUILD_HAIKU)
	if ( ( getuid() == 0 || geteuid() == 0 ) && !allowRoot )
	{
		printf( "LMMS cannot be run as root.\nUse \"--allowroot\" to override.\n\n" );
		return EXIT_FAILURE;
	}
#endif
	QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QCoreApplication * app = coreOnly ?
			new QCoreApplication( argc, argv ) :
					new gui::MainApplication(argc, argv);

	AudioEngine::qualitySettings qs(AudioEngine::qualitySettings::Interpolation::Linear);
	OutputSettings os(44100, 160, OutputSettings::BitDepth::Depth16Bit, OutputSettings::StereoMode::JointStereo);
	ProjectRenderer::ExportFileFormat eff = ProjectRenderer::ExportFileFormat::Wave;

	// second of two command-line parsing stages
	for( int i = 1; i < argc; ++i )
	{
		QString arg = argv[i];

		if (arg == "upgrade" || arg == "--upgrade" || arg  == "-u")
		{
			++i;

			if( i == argc )
			{
				return noInputFileError();
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
		else if (arg == "makebundle")
		{
			++i;

			if (i == argc)
			{
				return noInputFileError();
			}

			DataFile dataFile(QString::fromLocal8Bit(argv[i]));

			if (argc > i+1) // Project bundle file name given
			{
				printf("Making bundle\n");
				dataFile.writeFile(QString::fromLocal8Bit(argv[i+1]), true);
				return EXIT_SUCCESS;
			}
			else
			{
				return usageError("No project bundle name given");
			}
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
		else if( arg == "dump" || arg == "--dump" || arg  == "-d" )
		{
			++i;

			if( i == argc )
			{
				return noInputFileError();
			}


			QFile f( QString::fromLocal8Bit( argv[i] ) );
			f.open( QIODevice::ReadOnly );
			QString d = qUncompress( f.readAll() );
			printf( "%s\n", d.toUtf8().constData() );

			return EXIT_SUCCESS;
		}
		else if( arg == "compress" || arg == "--compress" )
		{
			++i;

			if( i == argc )
			{
				return noInputFileError();
			}

			QFile f( QString::fromLocal8Bit( argv[i] ) );
			f.open( QIODevice::ReadOnly );
			QByteArray d = qCompress( f.readAll() ) ;
			fwrite( d.constData(), sizeof(char), d.size(), stdout );

			return EXIT_SUCCESS;
		}
		else if( arg == "render" || arg == "--render" || arg == "-r" ||
			arg == "rendertracks" || arg == "--rendertracks" )
		{
			++i;

			if( i == argc )
			{
				return noInputFileError();
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
				return usageError( "No output file specified" );
			}


			renderOut = QString::fromLocal8Bit( argv[i] );
		}
		else if( arg == "--format" || arg == "-f" )
		{
			++i;

			if( i == argc )
			{
				return usageError( "No output format specified" );
			}


			const QString ext = QString( argv[i] );

			if( ext == "wav" )
			{
				eff = ProjectRenderer::ExportFileFormat::Wave;
			}
#ifdef LMMS_HAVE_OGGVORBIS
			else if( ext == "ogg" )
			{
				eff = ProjectRenderer::ExportFileFormat::Ogg;
			}
#endif
#ifdef LMMS_HAVE_MP3LAME
			else if( ext == "mp3" )
			{
				eff = ProjectRenderer::ExportFileFormat::MP3;
			}
#endif
			else if (ext == "flac")
			{
				eff = ProjectRenderer::ExportFileFormat::Flac;
			}
			else
			{
				return usageError( QString( "Invalid output format %1" ).arg( argv[i] ) );
			}
		}
		else if( arg == "--samplerate" || arg == "-s" )
		{
			++i;

			if( i == argc )
			{
				return usageError( "No samplerate specified" );
			}


			sample_rate_t sr = QString( argv[i] ).toUInt();
			if( sr >= 44100 && sr <= 192000 )
			{
				os.setSampleRate(sr);
			}
			else
			{
				return usageError( QString( "Invalid samplerate %1" ).arg( argv[i] ) );
			}
		}
		else if( arg == "--bitrate" || arg == "-b" )
		{
			++i;

			if( i == argc )
			{
				return usageError( "No bitrate specified" );
			}


			int br = QString( argv[i] ).toUInt();

			if( br >= 64 && br <= 384 )
			{
				os.setBitrate(br);
			}
			else
			{
				return usageError( QString( "Invalid bitrate %1" ).arg( argv[i] ) );
			}
		}
		else if( arg == "--mode" || arg == "-m" )
		{
			++i;

			if( i == argc )
			{
				return usageError( "No stereo mode specified" );
			}

			QString const mode( argv[i] );

			if( mode == "s" )
			{
				os.setStereoMode(OutputSettings::StereoMode::Stereo);
			}
			else if( mode == "j" )
			{
				os.setStereoMode(OutputSettings::StereoMode::JointStereo);
			}
			else if( mode == "m" )
			{
				os.setStereoMode(OutputSettings::StereoMode::Mono);
			}
			else
			{
				return usageError( QString( "Invalid stereo mode %1" ).arg( argv[i] ) );
			}
		}
		else if( arg =="--float" || arg == "-a" )
		{
			os.setBitDepth(OutputSettings::BitDepth::Depth32Bit);
		}
		else if( arg == "--interpolation" || arg == "-i" )
		{
			++i;

			if( i == argc )
			{
				return usageError( "No interpolation method specified" );
			}


			const QString ip = QString( argv[i] );

			if( ip == "linear" )
			{
		qs.interpolation = AudioEngine::qualitySettings::Interpolation::Linear;
			}
			else if( ip == "sincfastest" )
			{
		qs.interpolation = AudioEngine::qualitySettings::Interpolation::SincFastest;
			}
			else if( ip == "sincmedium" )
			{
		qs.interpolation = AudioEngine::qualitySettings::Interpolation::SincMedium;
			}
			else if( ip == "sincbest" )
			{
		qs.interpolation = AudioEngine::qualitySettings::Interpolation::SincBest;
			}
			else
			{
				return usageError( QString( "Invalid interpolation method %1" ).arg( argv[i] ) );
			}
		}
		else if( arg == "--import" )
		{
			++i;

			if( i == argc )
			{
				return usageError( "No file specified for importing" );
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
				return usageError( "No profile specified" );
			}


			profilerOutputFile = QString::fromLocal8Bit( argv[i] );
		}
		else if( arg == "--config" || arg == "-c" )
		{
			++i;

			if( i == argc )
			{
				return usageError( "No configuration file specified" );
			}

			configFile = QString::fromLocal8Bit( argv[i] );
		}
		else
		{
			if( argv[i][0] == '-' )
			{
				return usageError( QString( "Invalid option %1" ).arg( argv[i] ) );
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

	// Hidden settings
	MixHelpers::setNaNHandler( ConfigManager::inst()->value( "app",
						"nanhandler", "1" ).toInt() );

	// set language
	QString pos = ConfigManager::inst()->value( "app", "language" );
	if( pos.isEmpty() )
	{
		pos = QLocale::system().name().left( 2 );
	}

	// load actual translation for LMMS
	loadTranslation( pos );

	// load translation for Qt-widgets/-dialogs
#ifdef QT_TRANSLATIONS_DIR
	// load from the original path first
	loadTranslation(QString("qt_") + pos, QT_TRANSLATIONS_DIR);
#endif
	// override it with bundled/custom one, if exists
	loadTranslation(QString("qt_") + pos, ConfigManager::inst()->localeDir());

#if _POSIX_C_SOURCE >= 1 || _XOPEN_SOURCE || _POSIX_SOURCE
	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sa.sa_flags = SA_SIGINFO;
	if ( sigemptyset( &sa.sa_mask ) )
	{
		fprintf( stderr, "Signal initialization failed.\n" );
	}
	if ( sigaction( SIGPIPE, &sa, nullptr ) )
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
		auto r = new RenderManager(qs, os, eff, renderOut);
		QCoreApplication::instance()->connect( r,
				SIGNAL(finished()), SLOT(quit()));

		// timer for progress-updates
		auto t = new QTimer(r);
		r->connect( t, SIGNAL(timeout()),
				SLOT(updateConsoleProgress()));
		t->start( 200 );

		if( profilerOutputFile.isEmpty() == false )
		{
			Engine::audioEngine()->profiler().setOutputFile( profilerOutputFile );
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
		using namespace lmms::gui;

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
				MainWindow::tr( "Discard" ),
				MainWindow::tr( "Launch a default session and delete "
					"the restored files. This is not reversible." )
							) );

			mb.setIcon( QMessageBox::Warning );
			mb.setWindowIcon( embed::getIconPixmap( "icon_small" ) );
			mb.setWindowFlags( Qt::WindowCloseButtonHint );

			// setting all buttons to the same roles allows us
			// to have a custom layout
			auto discard = mb.addButton(MainWindow::tr("Discard"), QMessageBox::AcceptRole);
			auto recover = mb.addButton(MainWindow::tr("Recover"), QMessageBox::AcceptRole);

			// have a hidden exit button
			auto exit = mb.addButton("", QMessageBox::RejectRole);
			exit->setVisible(false);

			// set icons
			recover->setIcon( embed::getIconPixmap( "recover" ) );
			discard->setIcon( embed::getIconPixmap( "discard" ) );

			mb.setDefaultButton( recover );
			mb.setEscapeButton( exit );

			mb.exec();
			if( mb.clickedButton() == discard )
			{
				getGUI()->mainWindow()->sessionCleanup();
			}
			else if( mb.clickedButton() == recover ) // Recover
			{
				fileToLoad = recoveryFile;
				getGUI()->mainWindow()->setSession( MainWindow::SessionState::Recover );
			}
			else // Exit
			{
				return EXIT_SUCCESS;
			}
		}

		// first show the Main Window and then try to load given file

		// [Settel] workaround: showMaximized() doesn't work with
		// FVWM2 unless the window is already visible -> show() first
		getGUI()->mainWindow()->show();
		if( fullscreen )
		{
			getGUI()->mainWindow()->showMaximized();
		}

		// Handle macOS-style FileOpen QEvents
		QString queuedFile = static_cast<MainApplication *>( app )->queuedFile();
		if ( !queuedFile.isEmpty() ) {
			fileToLoad = queuedFile;
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
		// a new one.
		else if( ConfigManager::inst()->
				value( "app", "openlastproject" ).toInt() &&
			!ConfigManager::inst()->
				recentlyOpenedProjects().isEmpty() &&
				!recoveryFilePresent )
		{
			QString f = ConfigManager::inst()->
					recentlyOpenedProjects().first();
			QFileInfo recentFile( f );

			if ( recentFile.exists() &&
				recentFile.suffix().toLower() != "mpt" )
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
		if( autoSaveEnabled )
		{
			gui::getGUI()->mainWindow()->autoSaveTimerReset();
		}
	}

	const int ret = app->exec();
	delete app;

	if( destroyEngine )
	{
		Engine::destroy();
	}

	// ProjectRenderer::updateConsoleProgress() doesn't return line after render
	if( coreOnly )
	{
		printf( "\n" );
	}

#ifdef LMMS_BUILD_WIN32
	// Cleanup console
	HWND hConsole = GetConsoleWindow();
	if (hConsole)
	{
		SendMessage(hConsole, WM_CHAR, (WPARAM)VK_RETURN, (LPARAM)0);
		FreeConsole();
	}
#endif


	NotePlayHandleManager::free();

	return ret;
}
