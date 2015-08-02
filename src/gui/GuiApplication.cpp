/*
 * GuiApplication.cpp
 *
 * Copyright (c) 2014 Lukas W <lukaswhl/at/gmail.com>
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

#include "GuiApplication.h"

#include <QApplication>
#include <QFileInfo>
#include <QMessageBox>
#include <QSplashScreen>

#include "rtosc/rtosc.h"

#include "lmmsversion.h"

#include "LmmsStyle.h"
#include "LmmsPalette.h"

#include "AutomationEditor.h"
#include "BBEditor.h"
#include "ConfigManager.h"
#include "ControllerRackView.h"
#include "FxMixerView.h"
#include "ImportFilter.h"
#include "InstrumentTrack.h"
#include "MainWindow.h"
#include "PianoRoll.h"
#include "ProjectNotes.h"
#include "SongEditor.h"

#include <QApplication>
#include <QMessageBox>
#include <QSplashScreen>

GuiApplication* GuiApplication::s_instance = nullptr;

GuiApplication* GuiApplication::instance()
{
	return s_instance;
}


GuiApplication::GuiApplication(const QString &fileToLoad, const QString &fileToImport,
	bool fullscreen, bool exitAfterImport) :
	m_fileToLoad(fileToLoad),
	m_fileToImport(fileToImport),
	m_fullscreen(fullscreen),
	m_exitAfterImport(exitAfterImport)
{
	// prompt the user to create the LMMS working directory (e.g. ~/lmms) if it doesn't exist
	if ( !ConfigManager::inst()->hasWorkingDir() &&
		QMessageBox::question( NULL,
				tr( "Working directory" ),
				tr( "The LMMS working directory %1 does not "
				"exist. Create it now? You can change the directory "
				"later via Edit -> Settings." ).arg( ConfigManager::inst()->workingDir() ),
					QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes ) == QMessageBox::Yes)
	{
		ConfigManager::inst()->createWorkingDir();
	}
	// Add ourselves as a listener for Open Sound Control messages.
	// handle all Open Sound Control messages in the main gui thread
	connect(this, SIGNAL(receivedOscMessage(QByteArray)), this, 
		SLOT(processOscMsgInGuiThread(QByteArray)), Qt::QueuedConnection);
	// Add ourselves as a listener for OSC messages coming from the Engine
	Engine::messenger()->addGuiOscListener(this);
	this->listenInNewThread();

	// Init style and palette
	LmmsStyle* lmmsstyle = new LmmsStyle();
	QApplication::setStyle(lmmsstyle);

	LmmsPalette* lmmspal = new LmmsPalette(nullptr, lmmsstyle);
	QPalette* lpal = new QPalette(lmmspal->palette());

	QApplication::setPalette( *lpal );
	LmmsStyle::s_palette = lpal;

	// Show splash screen
	m_splashScreen = new QSplashScreen( embed::getIconPixmap( "splash" ) );
	m_splashScreen->show();

	QHBoxLayout layout;
	layout.setAlignment(Qt::AlignBottom);
	m_splashScreen->setLayout(&layout);

	// Create a left-aligned label for loading progress 
	// & a right-aligned label for version info
	m_loadingProgressLabel = new QLabel(m_splashScreen);
	QLabel *versionLabel = new QLabel(MainWindow::tr( "Version %1" ).arg( LMMS_VERSION ), m_splashScreen);

	m_loadingProgressLabel->setAlignment(Qt::AlignLeft);
	versionLabel->setAlignment(Qt::AlignRight);

	layout.addWidget(m_loadingProgressLabel);
	layout.addWidget(versionLabel);

	// may have long gaps between future frames, so force update now
	m_splashScreen->update();
	m_splashScreen->repaint();
	qApp->processEvents();

	// configure the initialization sequence
	//   (these all need to be queued connections so that we service user input regularly)
	connect(this, SIGNAL(postInitEngine()), this, SLOT(initMainWindow()), Qt::QueuedConnection);
	connect(this, SIGNAL(postInitMainWindow()), this, SLOT(initSongEditorWindow()), Qt::QueuedConnection);
	connect(this, SIGNAL(postInitSongEditorWindow()), this, SLOT(initFxMixerView()), Qt::QueuedConnection);
	connect(this, SIGNAL(postInitFxMixerView()), this, SLOT(initControllerRackView()), Qt::QueuedConnection);
	connect(this, SIGNAL(postInitControllerRackView()), this, SLOT(initProjectNotes()), Qt::QueuedConnection);
	connect(this, SIGNAL(postInitProjectNotes()), this, SLOT(initBbEditor()), Qt::QueuedConnection);
	connect(this, SIGNAL(postInitBbEditor()), this, SLOT(initPianoRoll()), Qt::QueuedConnection);
	connect(this, SIGNAL(postInitPianoRoll()), this, SLOT(initAutomationEditor()), Qt::QueuedConnection);
	connect(this, SIGNAL(postInitAutomationEditor()), this, SLOT(handleCtorOptions()), Qt::QueuedConnection);

	this->initEngine();
}

void GuiApplication::initEngine()
{
	// Init central engine which handles all components of LMMS
	Engine::init(false);

	s_instance = this;
	emit postInitEngine();
}

void GuiApplication::initMainWindow()
{
	displayInitProgress(tr("Preparing UI"));
	m_mainWindow = new MainWindow;
	connect(m_mainWindow, SIGNAL(destroyed(QObject*)), this, SLOT(childDestroyed(QObject*)));
	connect(m_mainWindow, SIGNAL(initProgress(const QString&)), 
		this, SLOT(displayInitProgress(const QString&)));

	emit postInitMainWindow();
}

void GuiApplication::initSongEditorWindow()
{
	displayInitProgress(tr("Preparing song editor"));
	m_songEditor = new SongEditorWindow(Engine::getSong());
	connect(m_songEditor, SIGNAL(destroyed(QObject*)), this, SLOT(childDestroyed(QObject*)));

	emit postInitSongEditorWindow();
}

void GuiApplication::initFxMixerView()
{
	displayInitProgress(tr("Preparing mixer"));
	m_fxMixerView = new FxMixerView;
	connect(m_fxMixerView, SIGNAL(destroyed(QObject*)), this, SLOT(childDestroyed(QObject*)));

	emit postInitFxMixerView();
}

void GuiApplication::initControllerRackView()
{
	displayInitProgress(tr("Preparing controller rack"));
	m_controllerRackView = new ControllerRackView;
	connect(m_controllerRackView, SIGNAL(destroyed(QObject*)), this, SLOT(childDestroyed(QObject*)));

	emit postInitControllerRackView();
}

void GuiApplication::initProjectNotes()
{
	displayInitProgress(tr("Preparing project notes"));
	m_projectNotes = new ProjectNotes;
	connect(m_projectNotes, SIGNAL(destroyed(QObject*)), this, SLOT(childDestroyed(QObject*)));

	emit postInitProjectNotes();
}

void GuiApplication::initBbEditor()
{
	displayInitProgress(tr("Preparing beat/bassline editor"));
	m_bbEditor = new BBEditor(Engine::getBBTrackContainer());
	connect(m_bbEditor, SIGNAL(destroyed(QObject*)), this, SLOT(childDestroyed(QObject*)));

	emit postInitBbEditor();
}

void GuiApplication::initPianoRoll()
{
	displayInitProgress(tr("Preparing piano roll"));
	m_pianoRoll = new PianoRollWindow();
	connect(m_pianoRoll, SIGNAL(destroyed(QObject*)), this, SLOT(childDestroyed(QObject*)));

	emit postInitPianoRoll();
}

void GuiApplication::initAutomationEditor()
{
	displayInitProgress(tr("Preparing automation editor"));
	m_automationEditor = new AutomationEditorWindow;
	connect(m_automationEditor, SIGNAL(destroyed(QObject*)), this, SLOT(childDestroyed(QObject*)));

	m_mainWindow->finalize();
	m_splashScreen->finish(m_mainWindow);

	m_loadingProgressLabel = nullptr;
	delete m_splashScreen;
	m_splashScreen = nullptr;


	emit postInitAutomationEditor();
}

void GuiApplication::handleCtorOptions()
{
	// recover a file?
	QString recoveryFile = ConfigManager::inst()->recoveryFile();

	if( QFileInfo(recoveryFile).exists() &&
		QMessageBox::question( gui->mainWindow(), MainWindow::tr( "Project recovery" ),
					MainWindow::tr( "It looks like the last session did not end properly. "
									"Do you want to recover the project of this session?" ),
					QMessageBox::Yes | QMessageBox::No ) == QMessageBox::Yes )
	{
		m_fileToLoad = recoveryFile;
	}

	// we try to load given file
	if( !m_fileToLoad.isEmpty() )
	{
		gui->mainWindow()->show();
		if( m_fullscreen )
		{
			gui->mainWindow()->showMaximized();
		}
		if( m_fileToLoad == recoveryFile )
		{
			Engine::getSong()->createNewProjectFromTemplate( m_fileToLoad );
		}
		else
		{
			Engine::getSong()->loadProject( m_fileToLoad );
		}
	}
	else if( !m_fileToImport.isEmpty() )
	{
		ImportFilter::import( m_fileToImport, Engine::getSong() );
		if( m_exitAfterImport )
		{
			QApplication::quit();
		}

		gui->mainWindow()->show();
		if( m_fullscreen )
		{
			gui->mainWindow()->showMaximized();
		}
	}
	else
	{
		// If enabled, open last project if there is one. Else, create
		// a new one.
		if( ConfigManager::inst()->
				value( "app", "openlastproject" ).toInt() &&
				!ConfigManager::inst()->recentlyOpenedProjects().isEmpty() )
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

		// [Settel] workaround: showMaximized() doesn't work with
		// FVWM2 unless the window is already visible -> show() first
		gui->mainWindow()->show();
		if( m_fullscreen )
		{
			gui->mainWindow()->showMaximized();
		}
	}
}

GuiApplication::~GuiApplication()
{
	InstrumentTrackView::cleanupWindowCache();
	s_instance = nullptr;
}


void GuiApplication::displayInitProgress(const QString &msg)
{
	if (m_loadingProgressLabel != nullptr)
	{
		m_loadingProgressLabel->setText(msg);
		// must force a UI update and process events, as there may be long gaps between processEvents() calls during init
		m_loadingProgressLabel->repaint();
		qApp->processEvents();
	}
}

void GuiApplication::childDestroyed(QObject *obj)
{
	// when any object that can be reached via gui->mainWindow(), gui->fxMixerView(), etc
	//   is destroyed, ensure that their accessor functions will return null instead of a garbage pointer.
	if (obj == m_mainWindow)
	{
		m_mainWindow = nullptr;
	}
	else if (obj == m_fxMixerView)
	{
		m_fxMixerView = nullptr;
	}
	else if (obj == m_songEditor)
	{
		m_songEditor = nullptr;
	}
	else if (obj == m_automationEditor)
	{
		m_automationEditor = nullptr;
	}
	else if (obj == m_bbEditor)
	{
		m_bbEditor = nullptr;
	}
	else if (obj == m_pianoRoll)
	{
		m_pianoRoll = nullptr;
	}
	else if (obj == m_projectNotes)
	{
		m_projectNotes = nullptr;
	}
	else if (obj == m_controllerRackView)
	{
		m_controllerRackView = nullptr;
	}
}

void GuiApplication::processMessage(const QByteArray &msg)
{
	emit receivedOscMessage(msg);
}

static bool checkOscArgs(const char *fmt, const QByteArray &msg)
{
	bool success = (strcmp(fmt, rtosc_argument_string(msg.data())) == 0);
	if (!success)
	{
		qDebug() << "GuiApplication: Bad OSC command";
	}
	return success;
}

void GuiApplication::processOscMsgInGuiThread(QByteArray msg)
{
	const char *data = msg.data();
	if (strcmp(Messenger::Endpoints::InitMsg, data) == 0)
	{
		if (checkOscArgs("s", msg))
		{
			displayInitProgress(QString::fromUtf8(rtosc_argument(data, 0).s));
		}
	}
	else if (strcmp(Messenger::Endpoints::Warning, data) == 0)
	{
		if (checkOscArgs("ss", msg))
		{
			QString brief = QString::fromUtf8(rtosc_argument(data, 0).s);
			QString warning = QString::fromUtf8(rtosc_argument(data, 1).s);
			QMessageBox::warning( NULL, 
				brief.isEmpty() ? MainWindow::tr( "Warning" ) : brief, warning);
		}
	}
	else if (strcmp(Messenger::Endpoints::Error, data) == 0)
	{
		if (checkOscArgs("ss", msg));
		{
			QString brief = QString::fromUtf8(rtosc_argument(data, 0).s);
			QString msg = QString::fromUtf8(rtosc_argument(data, 1).s);
			QMessageBox::critical( NULL, 
				brief.isEmpty() ? MainWindow::tr( "Error" ) : brief, msg);
		}
	}
	else
	{
		qDebug() << "GuiApplication::processMessage: Bad OSC path";
	}
}
