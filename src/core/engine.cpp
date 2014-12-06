/*
 * engine.cpp - implementation of LMMS' engine-system
 *
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "engine.h"
#include "AutomationEditor.h"
#include "bb_editor.h"
#include "bb_track_container.h"
#include "config_mgr.h"
#include "ControllerRackView.h"
#include "FxMixer.h"
#include "FxMixerView.h"
#include "InstrumentTrack.h"
#include "ladspa_2_lmms.h"
#include "MainWindow.h"
#include "Mixer.h"
#include "Pattern.h"
#include "PianoRoll.h"
#include "PresetPreviewPlayHandle.h"
#include "ProjectJournal.h"
#include "project_notes.h"
#include "Plugin.h"
#include "SongEditor.h"
#include "song.h"
#include "BandLimitedWave.h"


bool engine::s_hasGUI = true;
bool engine::s_suppressMessages = false;
float engine::s_framesPerTick;
Mixer* engine::s_mixer = NULL;
FxMixer * engine::s_fxMixer = NULL;
FxMixerView * engine::s_fxMixerView = NULL;
MainWindow * engine::s_mainWindow = NULL;
bbTrackContainer * engine::s_bbTrackContainer = NULL;
song * engine::s_song = NULL;
SongEditor* engine::s_songEditor = NULL;
AutomationEditor * engine::s_automationEditor = NULL;
bbEditor * engine::s_bbEditor = NULL;
PianoRoll* engine::s_pianoRoll = NULL;
projectNotes * engine::s_projectNotes = NULL;
ProjectJournal * engine::s_projectJournal = NULL;
ladspa2LMMS * engine::s_ladspaManager = NULL;
DummyTrackContainer * engine::s_dummyTC = NULL;
ControllerRackView * engine::s_controllerRackView = NULL;
QMap<QString, QString> engine::s_pluginFileHandling;




void engine::init( const bool _has_gui )
{
	s_hasGUI = _has_gui;

	// generate (load from file) bandlimited wavetables
	BandLimitedWave::generateWaves();

	initPluginFileHandling();

	s_projectJournal = new ProjectJournal;
	s_mixer = new Mixer;
	s_song = new song;
	s_fxMixer = new FxMixer;
	s_bbTrackContainer = new bbTrackContainer;

	s_ladspaManager = new ladspa2LMMS;

	s_projectJournal->setJournalling( true );

	s_mixer->initDevices();

	if( s_hasGUI )
	{
		s_mainWindow = new MainWindow;
		s_songEditor = new SongEditor( s_song );
		s_fxMixerView = new FxMixerView;
		s_controllerRackView = new ControllerRackView;
		s_projectNotes = new projectNotes;
		s_bbEditor = new bbEditor( s_bbTrackContainer );
		s_pianoRoll = new PianoRoll;
		s_automationEditor = new AutomationEditor;

		s_mainWindow->finalize();
	}

	PresetPreviewPlayHandle::init();
	s_dummyTC = new DummyTrackContainer;

	s_mixer->startProcessing();
}




void engine::destroy()
{
	s_projectJournal->stopAllJournalling();
	s_mixer->stopProcessing();

	deleteHelper( &s_projectNotes );
	deleteHelper( &s_songEditor );
	deleteHelper( &s_bbEditor );
	deleteHelper( &s_pianoRoll );
	deleteHelper( &s_automationEditor );
	deleteHelper( &s_fxMixerView );

	PresetPreviewPlayHandle::cleanup();
	InstrumentTrackView::cleanupWindowCache();

	s_song->clearProject();

	deleteHelper( &s_bbTrackContainer );
	deleteHelper( &s_dummyTC );

	deleteHelper( &s_mixer );
	deleteHelper( &s_fxMixer );

	deleteHelper( &s_ladspaManager );

	//delete configManager::inst();
	deleteHelper( &s_projectJournal );

	s_mainWindow = NULL;

	deleteHelper( &s_song );

	delete configManager::inst();
}




void engine::updateFramesPerTick()
{
	s_framesPerTick = s_mixer->processingSampleRate() * 60.0f * 4 /
				DefaultTicksPerTact / s_song->getTempo();
}




void engine::initPluginFileHandling()
{
	Plugin::DescriptorList pluginDescriptors;
	Plugin::getDescriptorsOfAvailPlugins( pluginDescriptors );
	for( Plugin::DescriptorList::ConstIterator it = pluginDescriptors.begin();
										it != pluginDescriptors.end(); ++it )
	{
		if( it->type == Plugin::Instrument )
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


