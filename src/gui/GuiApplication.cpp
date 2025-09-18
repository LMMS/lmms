/*
 * GuiApplication.cpp
 *
 * Copyright (c) 2014 Lukas W <lukaswhl/at/gmail.com>
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

#include "GuiApplication.h"

#include "lmmsversion.h"

#include "LmmsStyle.h"
#include "LmmsPalette.h"

#include "AutomationEditor.h"
#include "ConfigManager.h"
#include "ControllerRackView.h"
#include "MixerView.h"
#include "MainWindow.h"
#include "MicrotunerConfig.h"
#include "PatternEditor.h"
#include "PianoRoll.h"
#include "ProjectNotes.h"
#include "SongEditor.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QtGlobal>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QSplashScreen>
#include <QSocketNotifier>

#ifdef LMMS_BUILD_WIN32
#include <io.h>
#include <stdio.h>
#include <windows.h>
#else
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace lmms
{


namespace gui
{

GuiApplication* getGUI()
{
	return GuiApplication::instance();
}


GuiApplication* GuiApplication::s_instance = nullptr;

GuiApplication* GuiApplication::instance()
{
	return s_instance;
}

bool GuiApplication::isWayland()
{
	return QGuiApplication::platformName().contains("wayland");
}



GuiApplication::GuiApplication()
{
	// Immediately register our SIGINT handler
	createSocketNotifier();

	// prompt the user to create the LMMS working directory (e.g. ~/Documents/lmms) if it doesn't exist
	if ( !ConfigManager::inst()->hasWorkingDir() &&
		QMessageBox::question( nullptr,
				tr( "Working directory" ),
				tr( "The LMMS working directory %1 does not "
				"exist. Create it now? You can change the directory "
				"later via Edit -> Settings." ).arg( ConfigManager::inst()->workingDir() ),
					QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes ) == QMessageBox::Yes)
	{
		ConfigManager::inst()->createWorkingDir();
	}
	// Init style and palette
	QDir::addSearchPath("artwork", ConfigManager::inst()->themeDir());
	QDir::addSearchPath("artwork", ConfigManager::inst()->defaultThemeDir());
	QDir::addSearchPath("artwork", ":/artwork");

	auto lmmsstyle = new LmmsStyle();
	QApplication::setStyle(lmmsstyle);

	auto lmmspal = new LmmsPalette(nullptr, lmmsstyle);
	auto lpal = new QPalette(lmmspal->palette());

	QApplication::setPalette( *lpal );
	LmmsStyle::s_palette = lpal;

#ifdef LMMS_BUILD_APPLE
	QApplication::setAttribute(Qt::AA_DontShowIconsInMenus, true);
#endif

	// Show splash screen
	QSplashScreen splashScreen( embed::getIconPixmap( "splash" ) );
	splashScreen.setFixedSize(splashScreen.pixmap().size());
	splashScreen.show();

	QHBoxLayout layout;
	layout.setAlignment(Qt::AlignBottom);
	splashScreen.setLayout(&layout);

	// Create a left-aligned label for loading progress 
	// & a right-aligned label for version info
	QLabel loadingProgressLabel;
	m_loadingProgressLabel = &loadingProgressLabel;
	QLabel versionLabel(MainWindow::tr( "Version %1" ).arg( LMMS_VERSION ));

	loadingProgressLabel.setAlignment(Qt::AlignLeft);
	versionLabel.setAlignment(Qt::AlignRight);

	layout.addWidget(&loadingProgressLabel);
	layout.addWidget(&versionLabel);

	// may have long gaps between future frames, so force update now
	splashScreen.update();
	qApp->processEvents();

	connect(Engine::inst(), SIGNAL(initProgress(const QString&)), 
		this, SLOT(displayInitProgress(const QString&)));

	// Init central engine which handles all components of LMMS
	Engine::init(false);

	s_instance = this;

	displayInitProgress(tr("Preparing UI"));

	m_mainWindow = new MainWindow;
	connect(m_mainWindow, SIGNAL(destroyed(QObject*)), this, SLOT(childDestroyed(QObject*)));
	connect(m_mainWindow, SIGNAL(initProgress(const QString&)), 
		this, SLOT(displayInitProgress(const QString&)));

	displayInitProgress(tr("Preparing song editor"));
	m_songEditor = new SongEditorWindow(Engine::getSong());
	connect(m_songEditor, SIGNAL(destroyed(QObject*)), this, SLOT(childDestroyed(QObject*)));

	displayInitProgress(tr("Preparing mixer"));
	m_mixerView = new MixerView(Engine::mixer());
	connect(m_mixerView, SIGNAL(destroyed(QObject*)), this, SLOT(childDestroyed(QObject*)));

	displayInitProgress(tr("Preparing controller rack"));
	m_controllerRackView = new ControllerRackView;
	connect(m_controllerRackView, SIGNAL(destroyed(QObject*)), this, SLOT(childDestroyed(QObject*)));

	displayInitProgress(tr("Preparing project notes"));
	m_projectNotes = new ProjectNotes;
	connect(m_projectNotes, SIGNAL(destroyed(QObject*)), this, SLOT(childDestroyed(QObject*)));

	displayInitProgress(tr("Preparing microtuner"));
	m_microtunerConfig = new MicrotunerConfig;
	connect(m_microtunerConfig, SIGNAL(destroyed(QObject*)), this, SLOT(childDestroyed(QObject*)));

	displayInitProgress(tr("Preparing pattern editor"));
	m_patternEditor = new PatternEditorWindow(Engine::patternStore());
	connect(m_patternEditor, SIGNAL(destroyed(QObject*)), this, SLOT(childDestroyed(QObject*)));

	displayInitProgress(tr("Preparing piano roll"));
	m_pianoRoll = new PianoRollWindow();
	connect(m_pianoRoll, SIGNAL(destroyed(QObject*)), this, SLOT(childDestroyed(QObject*)));

	displayInitProgress(tr("Preparing automation editor"));
	m_automationEditor = new AutomationEditorWindow;
	connect(m_automationEditor, SIGNAL(destroyed(QObject*)), this, SLOT(childDestroyed(QObject*)));

	splashScreen.finish(m_mainWindow);
	m_mainWindow->finalize();

	m_loadingProgressLabel = nullptr;
}

GuiApplication::~GuiApplication()
{
	s_instance = nullptr;
}


void GuiApplication::displayInitProgress(const QString &msg)
{
	Q_ASSERT(m_loadingProgressLabel != nullptr);
	
	m_loadingProgressLabel->setText(msg);
	// must force a UI update and process events, as there may be long gaps between processEvents() calls during init
	m_loadingProgressLabel->repaint();
	qApp->processEvents();
}

void GuiApplication::childDestroyed(QObject *obj)
{
	// when any object that can be reached via getGUI()->mainWindow(), getGUI()->mixerView(), etc
	//   is destroyed, ensure that their accessor functions will return null instead of a garbage pointer.
	if (obj == m_mainWindow)
	{
		m_mainWindow = nullptr;
	}
	else if (obj == m_mixerView)
	{
		m_mixerView = nullptr;
	}
	else if (obj == m_songEditor)
	{
		m_songEditor = nullptr;
	}
	else if (obj == m_automationEditor)
	{
		m_automationEditor = nullptr;
	}
	else if (obj == m_patternEditor)
	{
		m_patternEditor = nullptr;
	}
	else if (obj == m_pianoRoll)
	{
		m_pianoRoll = nullptr;
	}
	else if (obj == m_projectNotes)
	{
		m_projectNotes = nullptr;
	}
	else if (obj == m_microtunerConfig)
	{
		m_microtunerConfig = nullptr;
	}
	else if (obj == m_controllerRackView)
	{
		m_controllerRackView = nullptr;
	}
}

/** \brief Called from main when SIGINT is fired
 *
 * Unix signal handlers can only call async-signal-safe functions:
 *  write(fd) --> QSocketNotifier --> SLOT sigintOccurred()
 *
 * See https://doc.qt.io/qt-6/unix-signals.html
 */
void GuiApplication::sigintHandler(int)
{
#ifdef LMMS_BUILD_WIN32
	char message[] = "Sorry, SIGINT is unhandled on this platform\n";
	std::ignore = _write(_fileno(stderr), message, sizeof(message));
#else
	char a = 1;
	std::ignore = ::write(s_sigintFd[0], &a, sizeof(a));
#endif
}

// Create our unix signal notifiers
void GuiApplication::createSocketNotifier()
{
#ifdef LMMS_BUILD_WIN32
	// no-op
#else
	if (::socketpair(AF_UNIX, SOCK_STREAM, 0, s_sigintFd))
	{
		qFatal("Couldn't create SIGINT socketpair");
		return;
	}

	// Listen on the file descriptor for SIGINT
	m_sigintNotifier = new QSocketNotifier(s_sigintFd[1], QSocketNotifier::Read, this);
	connect(m_sigintNotifier, SIGNAL(activated(QSocketDescriptor)), this, SLOT(sigintOccurred()), Qt::QueuedConnection);
#endif
}

// Handle the shutdown event
void GuiApplication::sigintOccurred()
{
	m_sigintNotifier->setEnabled(false);
	qDebug() << "Shutting down...";
	// cleanup, etc
	qApp->exit(3);
	m_sigintNotifier->setEnabled(true);
}

#ifdef LMMS_BUILD_WIN32
/*!
 * @brief Returns the Windows System font.
 */
QFont GuiApplication::getWin32SystemFont()
{
	NONCLIENTMETRICS metrics = { sizeof( NONCLIENTMETRICS ) };
	SystemParametersInfo( SPI_GETNONCLIENTMETRICS, sizeof( NONCLIENTMETRICS ), &metrics, 0 );
	int pointSize = metrics.lfMessageFont.lfHeight;
	if ( pointSize < 0 )
	{
		// height is in pixels, convert to points
		HDC hDC = GetDC( nullptr );
		pointSize = MulDiv(std::abs(pointSize), 72, GetDeviceCaps(hDC, LOGPIXELSY));
		ReleaseDC( nullptr, hDC );
	}

	return QFont( QString::fromUtf8( metrics.lfMessageFont.lfFaceName ), pointSize );
}
#endif


} // namespace gui

} // namespace lmms
