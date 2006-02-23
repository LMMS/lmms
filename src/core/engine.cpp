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


#include "bb_editor.h"
#include "engine.h"
#include "main_window.h"
#include "mixer.h"
#include "piano_roll.h"
#include "preset_preview_play_handle.h"
#include "project_notes.h"
#include "song_editor.h"


engine::engine( const bool _has_gui ) :
	m_hasGUI( _has_gui ),
	m_mixer( NULL ),
	m_mainWindow( NULL ),
	m_songEditor( NULL ),
	m_bbEditor( NULL ),
	m_pianoRoll( NULL )
{
	m_mainWindow = new mainWindow( this );
	m_mixer = new mixer( this );
	m_songEditor = new songEditor( this );
	m_projectNotes = new projectNotes( this );
	m_bbEditor = new bbEditor( this );
	m_pianoRoll = new pianoRoll( this );

	m_mainWindow->finalize();

	m_mixer->initDevices();
	m_mixer->startProcessing();
}




engine::~engine()
{
	m_mixer->stopProcessing();
	delete m_projectNotes;
	delete m_songEditor;
	delete m_bbEditor;
	delete m_pianoRoll;

	presetPreviewPlayHandle::cleanUp( this );

	// now we can clean up all allocated buffer
	//bufferAllocator::cleanUp( 0 );


	delete m_mixer;
	//delete configManager::inst();
	
}





engineObject::engineObject( engine * _engine ) :
	m_engine( _engine )
{
}




engineObject::~engineObject()
{
}


#endif
