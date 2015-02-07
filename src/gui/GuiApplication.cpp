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

#include "ConfigManager.h"

#include "AutomationEditor.h"
#include "BBEditor.h"
#include "ControllerRackView.h"
#include "FxMixerView.h"
#include "InstrumentTrack.h"
#include "MainWindow.h"
#include "PianoRoll.h"
#include "ProjectNotes.h"
#include "SongEditor.h"

#include <QDir>

#include <QApplication>
#include <QSplashScreen>

GuiApplication* GuiApplication::s_instance = nullptr;

GuiApplication* GuiApplication::instance()
{
	return s_instance;
}

GuiApplication::GuiApplication()
{
	// Init style and palette
	QDir::addSearchPath("icons", ConfigManager::inst()->artworkDir());
	QDir::addSearchPath("icons", ConfigManager::inst()->defaultArtworkDir());

	LmmsStyle* lmmsstyle = new LmmsStyle();
	QApplication::setStyle(lmmsstyle);

	LmmsPalette* lmmspal = new LmmsPalette(nullptr, lmmsstyle);
	QPalette* lpal = new QPalette(lmmspal->palette());

	QApplication::setPalette( *lpal );
	LmmsStyle::s_palette = lpal;

	// Show splash screen
	QSplashScreen splashScreen( QPixmap( "icons:splash.png" ) );
	splashScreen.show();
	splashScreen.showMessage( MainWindow::tr( "Version %1" ).arg( LMMS_VERSION ),
								Qt::AlignRight | Qt::AlignBottom, Qt::white );
	qApp->processEvents();

	// Init central engine which handles all components of LMMS
	Engine::init();

	s_instance = this;

	m_mainWindow = new MainWindow;

	m_songEditor = new SongEditorWindow(Engine::getSong());
	m_fxMixerView = new FxMixerView;
	m_controllerRackView = new ControllerRackView;
	m_projectNotes = new ProjectNotes;
	m_bbEditor = new BBEditor(Engine::getBBTrackContainer());
	m_pianoRoll = new PianoRollWindow();
	m_automationEditor = new AutomationEditorWindow;

	m_mainWindow->finalize();
	splashScreen.finish(m_mainWindow);
}

GuiApplication::~GuiApplication()
{
	InstrumentTrackView::cleanupWindowCache();
	s_instance = nullptr;
}
