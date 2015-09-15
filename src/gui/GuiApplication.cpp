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

#include <memory>

#include <QApplication>
#include <QFileInfo>
#include <QMessageBox>
#include <QSplashScreen>

#include <lo/lo_cpp.h>

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
#include "Messenger.h"
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
	m_mainWindow(nullptr),
	m_fxMixerView(nullptr),
	m_songEditor(nullptr),
	m_automationEditor(nullptr),
	m_bbEditor(nullptr),
	m_pianoRoll(nullptr),
	m_projectNotes(nullptr),
	m_controllerRackView(nullptr),
	m_loadingProgressLabel(nullptr),
	m_splashScreen(nullptr),
	m_fileToLoad(fileToLoad),
	m_fileToImport(fileToImport),
	m_fullscreen(fullscreen),
	m_exitAfterImport(exitAfterImport),
	m_oscListener(NULL)
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

	// choose a random unused port by instantiating and killing a server
	lo_server tempServ = lo_server_new(NULL, NULL);
	std::string portStdStr = QString::number(lo_server_get_port(tempServ)).toStdString();
	lo_server_free(tempServ);
	const char *port = portStdStr.c_str();
	// Create the actual server, and place it in a new thread
	m_oscListener = new lo::ServerThread(port);


	// in order to establish a queued connection to a function taking a nontrivial type,
	// we must declare the type
	qRegisterMetaType<QVector<float> >("QVector<float>");

	// configure all the OSC endpoint mappings
	m_oscListener->add_method(Messenger::Endpoints::Warning, "ss",
		[this](lo_arg **args, int numArgs)
		{
			QMetaObject::invokeMethod(this, "onOscRecvWarning",
				Qt::QueuedConnection,
				Q_ARG(QString, QString::fromUtf8(&args[0]->s)),
				Q_ARG(QString, QString::fromUtf8(&args[1]->s)));
		});
	m_oscListener->add_method(Messenger::Endpoints::Error, "ss",
		[this](lo_arg **args, int numArgs)
		{
			QMetaObject::invokeMethod(this, "onOscRecvError",
				Qt::QueuedConnection,
				Q_ARG(QString, QString::fromUtf8(&args[0]->s)),
				Q_ARG(QString, QString::fromUtf8(&args[1]->s)));
		});
	m_oscListener->add_method(Messenger::Endpoints::WaveTableInit, "",
		[this]()
		{
			QMetaObject::invokeMethod(this, "displayInitProgress",
				Qt::QueuedConnection,
				Q_ARG(QString, tr("Generated wavetables") ));
		});
	m_oscListener->add_method(Messenger::Endpoints::MixerDevInit, "",
		[this]()
		{
			QMetaObject::invokeMethod(this, "displayInitProgress",
				Qt::QueuedConnection,
				Q_ARG(QString, tr("Opened audio & midi devices") ));
		});
	m_oscListener->add_method(Messenger::Endpoints::MixerProcessingStart, "",
		[this]()
		{
			QMetaObject::invokeMethod(this, "displayInitProgress",
				Qt::QueuedConnection,
				Q_ARG(QString, tr("Started mixer threads") ));
		});
	m_oscListener->add_method(Messenger::Endpoints::FxMixerPeaks, NULL,
		[this](const char *types, lo_arg ** args, int numArgs)
		{
			if (numArgs % 2 != 0)
			{
				// Peak data must be paired
				qDebug() << "GuiApplication: Invalid mixer peak data";
				return;
			}
			// must manually validate the argument types
			for (int i=0; i<numArgs; ++i)
			{
				if (types[i] != 'f')
				{
					qDebug() << "GuiApplication: Invalid mixer peak data";
					return;
				}
			}
			QVector<float> peaks;
			peaks.reserve(numArgs);
			for (int i=0; i<numArgs; ++i)
			{
				peaks.append(args[i]->f);
			}
			QMetaObject::invokeMethod(this, "updateMixerPeaks",
				Qt::QueuedConnection,
				Q_ARG(QVector<float>, peaks ));
		});
	m_oscListener->start();

	// Add ourselves as a listener for OSC messages coming from the Engine
	Messenger *messenger(Engine::messenger());
	messenger->addListener(Messenger::Endpoints::Warning, lo_address_new("localhost", port));
	messenger->addListener(Messenger::Endpoints::Error, lo_address_new("localhost", port));
	messenger->addListener(Messenger::Endpoints::WaveTableInit, lo_address_new("localhost", port));
	messenger->addListener(Messenger::Endpoints::MixerDevInit, lo_address_new("localhost", port));
	messenger->addListener(Messenger::Endpoints::MixerProcessingStart, lo_address_new("localhost", port));
	messenger->addListener(Messenger::Endpoints::FxMixerPeaks, lo_address_new("localhost", port));


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
		Engine::getSong()->createNewProject();

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


void GuiApplication::displayInitProgress(QString msg)
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

void GuiApplication::onOscRecvWarning(QString brief, QString msg)
{
	QMessageBox::warning( NULL,
		brief.isEmpty() ? MainWindow::tr( "Warning" ) : brief, msg);
}

void GuiApplication::onOscRecvError(QString brief, QString msg)
{
	QMessageBox::critical( NULL,
		brief.isEmpty() ? MainWindow::tr( "Error" ) : brief, msg);
}

void GuiApplication::updateMixerPeaks(QVector<float> peaks)
{
	if (fxMixerView())
	{
		fxMixerView()->gotChannelPeaks(peaks.size()/2, peaks.data());
	}
}
