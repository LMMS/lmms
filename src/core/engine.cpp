#ifndef SINGLE_SOURCE_COMPILE

/*
 * engine.cpp - implementation of LMMS' engine-system
 *
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#include "automation_editor.h"
#include "bb_editor.h"
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


engine::engine( const bool _has_gui ) :
	m_hasGUI( _has_gui ),
	m_mixer( NULL ),
	m_mainWindow( NULL ),
	m_songEditor( NULL ),
	m_automationEditor( NULL ),
	m_bbEditor( NULL ),
	m_pianoRoll( NULL ),
	m_projectJournal( NULL )
{
	m_projectJournal = new projectJournal( this );
	m_mainWindow = new mainWindow( this );
	m_mixer = new mixer( this );
	m_songEditor = new songEditor( this );
	m_projectNotes = new projectNotes( this );
	m_bbEditor = new bbEditor( this );
	m_pianoRoll = new pianoRoll( this );
	m_automationEditor = new automationEditor( this );

#ifdef LADSPA_SUPPORT
	m_ladspaManager = new ladspa2LMMS( this );
#endif
	
	m_mixer->initDevices();

	m_mainWindow->finalize();

	m_mixer->startProcessing();
}




engine::~engine()
{
}




void engine::close( void )
{
	m_mixer->stopProcessing();

	delete m_projectNotes;
	m_projectNotes = NULL;
	delete m_songEditor;
	m_songEditor = NULL;
	delete m_bbEditor;
	m_bbEditor = NULL;
	delete m_pianoRoll;
	m_pianoRoll = NULL;
	delete m_automationEditor;
	m_automationEditor = NULL;

	presetPreviewPlayHandle::cleanUp( this );

	// now we can clean up all allocated buffer
	//bufferAllocator::cleanUp( 0 );


	delete m_mixer;
	m_mixer = NULL;
	//delete configManager::inst();
	delete m_projectJournal;
	m_projectJournal = NULL;
	m_mainWindow = NULL;
}




void engine::updateFramesPerTact64th( void )
{
	m_frames_per_tact64th = m_mixer->sampleRate() * 60.0f * BEATS_PER_TACT
					/ 64.0f / m_songEditor->getTempo();
}





engineObject::engineObject( engine * _engine ) :
	m_engine( _engine )
{
}




engineObject::~engineObject()
{
}


#endif
