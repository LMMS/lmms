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

#include "lmmsversion.h"

#include "LmmsStyle.h"
#include "LmmsPalette.h"

#include "AutomationEditor.h"
#include "BBEditor.h"
#include "ControllerRackView.h"
#include "FxMixerView.h"
#include "InstrumentTrack.h"
#include "MainWindow.h"
#include "PianoRoll.h"
#include "ProjectNotes.h"
#include "SongEditor.h"

#include <QApplication>
#include <QSplashScreen>


namespace lmms
{


GuiApplication* GuiApplication::s_instance = nullptr;

GuiApplication* GuiApplication::instance()
{
	return s_instance;
}


GuiApplication::GuiApplication()
{
	// Init style and palette
	LmmsStyle* lmmsstyle = new LmmsStyle();
	QApplication::setStyle(lmmsstyle);

	LmmsPalette* lmmspal = new LmmsPalette(nullptr, lmmsstyle);
	QPalette* lpal = new QPalette(lmmspal->palette());

	QApplication::setPalette( *lpal );
	LmmsStyle::s_palette = lpal;

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
	Engine::init();

	s_instance = this;

	displayInitProgress(tr("Preparing UI"));

	m_mainWindow = new MainWindow;
	connect(m_mainWindow, SIGNAL(initProgress(const QString&)), 
		this, SLOT(displayInitProgress(const QString&)));

	displayInitProgress(tr("Preparing song editor"));
	m_songEditor = new SongEditorWindow(Engine::getSong());
	displayInitProgress(tr("Preparing mixer"));
	m_fxMixerView = new FxMixerView;
	displayInitProgress(tr("Preparing controller rack"));
	m_controllerRackView = new ControllerRackView;
	displayInitProgress(tr("Preparing project notes"));
	m_projectNotes = new ProjectNotes;
	displayInitProgress(tr("Preparing beat/bassline editor"));
	m_bbEditor = new BBEditor(Engine::getBBTrackContainer());
	displayInitProgress(tr("Preparing piano roll"));
	m_pianoRoll = new PianoRollWindow();
	displayInitProgress(tr("Preparing automation editor"));
	m_automationEditor = new AutomationEditorWindow;

	m_mainWindow->finalize();
	splashScreen.finish(m_mainWindow);

	m_loadingProgressLabel = nullptr;
}

GuiApplication::~GuiApplication()
{
	InstrumentTrackView::cleanupWindowCache();
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



}
