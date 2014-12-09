/*
 * Engine.cpp - implementation of LMMS' engine-system
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


#include "Engine.h"
#include "AutomationEditor.h"
#include "BBEditor.h"
#include "BBTrackContainer.h"
#include "ConfigManager.h"
#include "ControllerRackView.h"
#include "FxMixer.h"
#include "FxMixerView.h"
#include "InstrumentTrack.h"
#include "Ladspa2LMMS.h"
#include "MainWindow.h"
#include "Mixer.h"
#include "Pattern.h"
#include "PianoRoll.h"
#include "PresetPreviewPlayHandle.h"
#include "ProjectJournal.h"
#include "ProjectNotes.h"
#include "Plugin.h"
#include "SongEditor.h"
#include "Song.h"
#include "BandLimitedWave.h"


bool Engine::s_hasGUI = true;
bool Engine::s_suppressMessages = false;
float Engine::s_framesPerTick;
Mixer* Engine::s_mixer = NULL;
FxMixer * Engine::s_fxMixer = NULL;
FxMixerView * Engine::s_fxMixerView = NULL;
MainWindow * Engine::s_mainWindow = NULL;
BBTrackContainer * Engine::s_bbTrackContainer = NULL;
Song * Engine::s_song = NULL;
SongEditor* Engine::s_songEditor = NULL;
AutomationEditor * Engine::s_automationEditor = NULL;
BBEditor * Engine::s_bbEditor = NULL;
PianoRoll* Engine::s_pianoRoll = NULL;
ProjectNotes * Engine::s_projectNotes = NULL;
ProjectJournal * Engine::s_projectJournal = NULL;
Ladspa2LMMS * Engine::s_ladspaManager = NULL;
DummyTrackContainer * Engine::s_dummyTC = NULL;
ControllerRackView * Engine::s_controllerRackView = NULL;
QMap<QString, QString> Engine::s_pluginFileHandling;




void Engine::init( const bool _has_gui )
{
	s_hasGUI = _has_gui;

	// generate (load from file) bandlimited wavetables
	BandLimitedWave::generateWaves();

	initPluginFileHandling();

	s_projectJournal = new ProjectJournal;
	s_mixer = new Mixer;
	s_song = new Song;
	s_fxMixer = new FxMixer;
	s_bbTrackContainer = new BBTrackContainer;

	s_ladspaManager = new Ladspa2LMMS;

	s_projectJournal->setJournalling( true );

	s_mixer->initDevices();

	if( s_hasGUI )
	{
		s_mainWindow = new MainWindow;
		s_songEditor = new SongEditor( s_song );
		s_fxMixerView = new FxMixerView;
		s_controllerRackView = new ControllerRackView;
		s_projectNotes = new ProjectNotes;
		s_bbEditor = new BBEditor( s_bbTrackContainer );
		s_pianoRoll = new PianoRoll;
		s_automationEditor = new AutomationEditor;

		s_mainWindow->finalize();
	}

	PresetPreviewPlayHandle::init();
	s_dummyTC = new DummyTrackContainer;

	s_mixer->startProcessing();
}




void Engine::destroy()
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

	//delete ConfigManager::inst();
	deleteHelper( &s_projectJournal );

	s_mainWindow = NULL;

	deleteHelper( &s_song );

	delete ConfigManager::inst();
}




void Engine::updateFramesPerTick()
{
	s_framesPerTick = s_mixer->processingSampleRate() * 60.0f * 4 /
				DefaultTicksPerTact / s_song->getTempo();
}




void Engine::initPluginFileHandling()
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


