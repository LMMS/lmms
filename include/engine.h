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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#ifndef _ENGINE_H
#define _ENGINE_H

#include "qt3support.h"


class bbEditor;
class mainWindow;
class mixer;
class pianoRoll;
class projectNotes;
class songEditor;


class engine
{
public:
	engine( const bool _has_gui = TRUE );
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


private:
	bool m_hasGUI;

	mixer * m_mixer;
	mainWindow * m_mainWindow;
	songEditor * m_songEditor;
	bbEditor * m_bbEditor;
	pianoRoll * m_pianoRoll;
	projectNotes * m_projectNotes;

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
