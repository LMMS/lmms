#ifndef SINGLE_SOURCE_COMPILE

/*
 * engine.cpp - implementation of LMMS' engine-system
 *
 * Copyright (c) 2006-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QtCore/QDir>

#include "engine.h"
#include "automation_editor.h"
#include "bb_editor.h"
#include "bb_track_container.h"
#include "config_mgr.h"
#include "controller_rack_view.h"
#include "fx_mixer.h"
#include "fx_mixer_view.h"
#include "ladspa_2_lmms.h"
#include "main_window.h"
#include "mixer.h"
#include "pattern.h"
#include "piano_roll.h"
#include "preset_preview_play_handle.h"
#include "project_journal.h"
#include "project_notes.h"
#include "resources_db.h"
#include "song_editor.h"
#include "song.h"


bool engine::s_hasGUI = true;
bool engine::s_suppressMessages = false;
float engine::s_framesPerTick;
mixer * engine::s_mixer = NULL;
fxMixer * engine::s_fxMixer = NULL;
fxMixerView * engine::s_fxMixerView = NULL;
mainWindow * engine::s_mainWindow = NULL;
bbTrackContainer * engine::s_bbTrackContainer = NULL;
song * engine::s_song = NULL;
ResourcesDB * engine::s_resourcesDB = NULL;
songEditor * engine::s_songEditor = NULL;
automationEditor * engine::s_automationEditor = NULL;
bbEditor * engine::s_bbEditor = NULL;
pianoRoll * engine::s_pianoRoll = NULL;
projectNotes * engine::s_projectNotes = NULL;
projectJournal * engine::s_projectJournal = NULL;
ladspa2LMMS * engine::s_ladspaManager = NULL;
dummyTrackContainer * engine::s_dummyTC = NULL;
controllerRackView * engine::s_controllerRackView = NULL;
QMap<QString, QString> engine::s_pluginFileHandling;
LmmsStyle * engine::s_lmmsStyle = NULL;




void engine::init( const bool _has_gui )
{
	s_hasGUI = _has_gui;

	initPluginFileHandling();

	s_projectJournal = new projectJournal;
	s_mixer = new mixer;
	s_song = new song;
	s_resourcesDB = new ResourcesDB( configManager::inst()->workingDir() +
						QDir::separator() +
							".resourcesdb.xml" );
	s_fxMixer = new fxMixer;
	s_bbTrackContainer = new bbTrackContainer;

	s_ladspaManager = new ladspa2LMMS;

	s_projectJournal->setJournalling( TRUE );

	s_mixer->initDevices();

	if( s_hasGUI )
	{
		s_mainWindow = new mainWindow;
		s_songEditor = new songEditor( s_song, s_songEditor );
		s_bbEditor = new bbEditor( s_bbTrackContainer );
		s_pianoRoll = new pianoRoll;
		s_automationEditor = new automationEditor;
		s_fxMixerView = new fxMixerView;
		s_controllerRackView = new controllerRackView;
		s_projectNotes = new projectNotes;

		s_mainWindow->finalize();
	}

	presetPreviewPlayHandle::init();
	s_dummyTC = new dummyTrackContainer;

	s_mixer->startProcessing();
}




void engine::destroy( void )
{
	s_mixer->stopProcessing();

	delete s_projectNotes;
	s_projectNotes = NULL;
	delete s_songEditor;
	s_songEditor = NULL;
	delete s_bbEditor;
	s_bbEditor = NULL;
	delete s_pianoRoll;
	s_pianoRoll = NULL;
	delete s_automationEditor;
	s_automationEditor = NULL;

	delete s_fxMixerView;
	s_fxMixerView = NULL;

	presetPreviewPlayHandle::cleanup();
	instrumentTrackView::cleanupWindowPool();

	s_song->clearProject();
	delete s_bbTrackContainer;
	s_bbTrackContainer = NULL;
	delete s_dummyTC;
	s_dummyTC = NULL;

	delete s_mixer;
	s_mixer = NULL;
	delete s_fxMixer;
	s_fxMixer = NULL;

	delete s_ladspaManager;

	//delete configManager::inst();
	delete s_projectJournal;
	s_projectJournal = NULL;
	s_mainWindow = NULL;

	delete s_song;
	s_song = NULL;

	delete s_resourcesDB;
	s_resourcesDB = NULL;

	delete configManager::inst();
}




void engine::updateFramesPerTick( void )
{
	s_framesPerTick = s_mixer->processingSampleRate() * 60.0f * 4 /
				DefaultTicksPerTact / s_song->getTempo();
}




void engine::initPluginFileHandling( void )
{
	QVector<plugin::descriptor> pluginDescriptors;
	plugin::getDescriptorsOfAvailPlugins( pluginDescriptors );
	for( QVector<plugin::descriptor>::iterator it =
						pluginDescriptors.begin();
					it != pluginDescriptors.end(); ++it )
	{
		if( it->type == plugin::Instrument )
		{
			const QStringList & ext =
				QString( it->supportedFileTypes ).
							split( QChar( ',' ) );
			for( QStringList::const_iterator itExt = ext.begin();
						itExt != ext.end(); ++itExt )
			{
				s_pluginFileHandling[*itExt] = it->name;
			}
		}
	}
}




#endif
