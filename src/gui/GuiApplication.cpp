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
#include "InstrumentTrack.h"
#include "MainWindow.h"
#include "MicrotunerConfig.h"
#include "PatternEditor.h"
#include "PianoRoll.h"
#include "ProjectNotes.h"
#include "SongEditor.h"

#include <QApplication>
#include <QDir>
#include <QtGlobal>
#include <QMessageBox>
#include <QSplashScreen>

#ifdef LMMS_BUILD_WIN32
#include <windows.h>
#endif

GuiApplication* GuiApplication::s_instance = nullptr;

GuiApplication* GuiApplication::instance()
{
	return s_instance;
}

GuiApplication* getGUI()
{
	return GuiApplication::instance();
}

GuiApplication::GuiApplication()
{
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

	LmmsStyle* lmmsstyle = new LmmsStyle();
	QApplication::setStyle(lmmsstyle);

	LmmsPalette* lmmspal = new LmmsPalette(nullptr, lmmsstyle);
	QPalette* lpal = new QPalette(lmmspal->palette());

	QApplication::setPalette( *lpal );
	LmmsStyle::s_palette = lpal;

#ifdef LMMS_BUILD_APPLE
	QApplication::setAttribute(Qt::AA_DontShowIconsInMenus, true);
#endif

	// Show splash screen
	QSplashScreen splashScreen( embed::getIconPixmap( "splash" ) );
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
	m_mixerView = new MixerView;
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
		pointSize = MulDiv( abs( pointSize ), 72, GetDeviceCaps( hDC, LOGPIXELSY ) );
		ReleaseDC( nullptr, hDC );
	}

	return QFont( QString::fromUtf8( metrics.lfMessageFont.lfFaceName ), pointSize );
}
#endif
