/*
 * engine.h - engine-system of LMMS
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */


#ifndef _ENGINE_H
#define _ENGINE_H

class automationEditor;
class bbEditor;
class projectJournal;
class mainWindow;
class mixer;
class pianoRoll;
class projectNotes;
class songEditor;

#include "ladspa_manager.h"
#ifdef LADSPA_SUPPORT
class ladspa2LMMS;
#endif

class engine
{
public:
	engine( const bool _has_gui = true );
	engine( const engine & _engine );
	~engine();

	engine * duplicate( const engine * _engine )
	{
		return( new engine( *_engine ) );
	}

	inline bool hasGUI( void ) const
	{
		return( m_hasGUI );
	}

	inline mixer * getMixer( void )
	{
		return( m_mixer );
	}

	inline const mixer * getMixer( void ) const
	{
		return( m_mixer );
	}

	inline mainWindow * getMainWindow( void )
	{
		return( m_mainWindow );
	}

	inline songEditor * getSongEditor( void )
	{
		return( m_songEditor );
	}

	inline const songEditor * getSongEditor( void ) const
	{
		return( m_songEditor );
	}

	inline bbEditor * getBBEditor( void )
	{
		return( m_bbEditor );
	}

	inline pianoRoll * getPianoRoll( void )
	{
		return( m_pianoRoll );
	}

	inline projectNotes * getProjectNotes( void )
	{
		return( m_projectNotes );
	}

	inline projectJournal * getProjectJournal( void )
	{
		return( m_projectJournal );
	}

	inline automationEditor * getAutomationEditor( void )
	{
		return( m_automationEditor );
	}

#ifdef LADSPA_SUPPORT
	inline ladspa2LMMS * getLADSPAManager( void )
	{
		return( m_ladspaManager );
	}
#endif
	
	void close( void );

	float framesPerTact64th( void ) const
	{
		return( m_frames_per_tact64th );
	}
	void updateFramesPerTact64th( void );


private:
	bool m_hasGUI;
	float m_frames_per_tact64th;

	mixer * m_mixer;
	mainWindow * m_mainWindow;
	songEditor * m_songEditor;
	automationEditor * m_automationEditor;
	bbEditor * m_bbEditor;
	pianoRoll * m_pianoRoll;
	projectNotes * m_projectNotes;
	projectJournal * m_projectJournal;
	
#ifdef LADSPA_SUPPORT
	ladspa2LMMS * m_ladspaManager;
#endif
	
} ;



class engineObject
{
public:
	engineObject( engine * _engine );
	~engineObject();

	inline engine * eng( void )
	{
		return( m_engine );
	}

	inline const engine * eng( void ) const
	{
		return( m_engine );
	}

	inline bool hasGUI( void ) const
	{
		return( m_engine->hasGUI() );
	}


private:
	engine * m_engine;

} ;


#endif
