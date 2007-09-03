#ifndef SINGLE_SOURCE_COMPILE

/*
 * engine.cpp - implementation of LMMS' engine-system
 *
 * Copyright (c) 2006-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "automation_editor.h"
#include "bb_editor.h"
#include "config_mgr.h"
#include "project_journal.h"
#include "engine.h"
#include "main_window.h"
#include "mixer.h"
#include "pattern.h"
#include "piano_roll.h"
#include "preset_preview_play_handle.h"
#include "project_notes.h"
#include "song_editor.h"

#ifdef LADSPA_SUPPORT
#include "ladspa_2_lmms.h"
#endif


bool engine::s_hasGUI = TRUE;
float engine::s_frames_per_tact64th;
mixer * engine::s_mixer;
mainWindow * engine::s_mainWindow;
songEditor * engine::s_songEditor;
automationEditor * engine::s_automationEditor;
bbEditor * engine::s_bbEditor;
pianoRoll * engine::s_pianoRoll;
projectNotes * engine::s_projectNotes;
projectJournal * engine::s_projectJournal;
#ifdef LADSPA_SUPPORT
ladspa2LMMS * engine::s_ladspaManager;
#endif
QMap<QString, QString> engine::s_sample_extensions;




void engine::init( const bool _has_gui )
{
	s_hasGUI = _has_gui;

	load_extensions();

	s_projectJournal = new projectJournal;
	s_mainWindow = new mainWindow;
	s_mixer = new mixer;
	s_songEditor = new songEditor;
	s_projectNotes = new projectNotes;
	s_bbEditor = new bbEditor;
	s_pianoRoll = new pianoRoll;
	s_automationEditor = new automationEditor;

#ifdef LADSPA_SUPPORT
	s_ladspaManager = new ladspa2LMMS;
#endif
	
	s_mixer->initDevices();

	s_mainWindow->finalize();

	presetPreviewPlayHandle::init();

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

#ifdef LADSPA_SUPPORT
	delete s_ladspaManager;
#endif

	presetPreviewPlayHandle::cleanUp();

	delete s_mixer;
	s_mixer = NULL;
	//delete configManager::inst();
	delete s_projectJournal;
	s_projectJournal = NULL;
	s_mainWindow = NULL;

	delete configManager::inst();
}




void engine::updateFramesPerTact64th( void )
{
	s_frames_per_tact64th = s_mixer->sampleRate() * 60.0f * BEATS_PER_TACT
					/ 64.0f / s_songEditor->getTempo();
}




void engine::load_extensions( void )
{
	QVector<plugin::descriptor> pluginDescriptors;
	plugin::getDescriptorsOfAvailPlugins( pluginDescriptors );
	for( QVector<plugin::descriptor>::iterator it =
						pluginDescriptors.begin();
					it != pluginDescriptors.end(); ++it )
	{
		if( it->sub_plugin_features )
		{
			if( it->type == plugin::Instrument )
			{
				const QStringList & ext =
						it->sub_plugin_features
							->supportedExtensions();
				for( QStringList::const_iterator itExt =
								ext.begin();
						itExt != ext.end(); ++itExt )
				{
					s_sample_extensions[*itExt] = it->name;
				}
			}
		}
	}
}




#endif
