/*
 * engine.h - engine-system of LMMS
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


#ifndef _ENGINE_H
#define _ENGINE_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <QtCore/QMap>

class automationEditor;
class bbEditor;
class projectJournal;
class mainWindow;
class mixer;
class pianoRoll;
class projectNotes;
class songEditor;
class ladspa2LMMS;


class engine
{
public:
	static void init( const bool _has_gui = true );
	static void destroy( void );

	static bool hasGUI( void )
	{
		return( s_hasGUI );
	}

	static mixer * getMixer( void )
	{
		return( s_mixer );
	}

	static mainWindow * getMainWindow( void )
	{
		return( s_mainWindow );
	}

	static songEditor * getSongEditor( void )
	{
		return( s_songEditor );
	}

	static bbEditor * getBBEditor( void )
	{
		return( s_bbEditor );
	}

	static pianoRoll * getPianoRoll( void )
	{
		return( s_pianoRoll );
	}

	static projectNotes * getProjectNotes( void )
	{
		return( s_projectNotes );
	}

	static projectJournal * getProjectJournal( void )
	{
		return( s_projectJournal );
	}

	static automationEditor * getAutomationEditor( void )
	{
		return( s_automationEditor );
	}

	static ladspa2LMMS * getLADSPAManager( void )
	{
		return( s_ladspaManager );
	}

	static float framesPerTact64th( void )
	{
		return( s_framesPerTact64th );
	}
	static void updateFramesPerTact64th( void );

	static const QMap<QString, QString> & sampleExtensions( void )
	{
		return( s_sampleExtensions );
	}


private:
	static bool s_hasGUI;
	static float s_framesPerTact64th;

	static mixer * s_mixer;
	static mainWindow * s_mainWindow;
	static songEditor * s_songEditor;
	static automationEditor * s_automationEditor;
	static bbEditor * s_bbEditor;
	static pianoRoll * s_pianoRoll;
	static projectNotes * s_projectNotes;
	static projectJournal * s_projectJournal;
	static ladspa2LMMS * s_ladspaManager;

	static QMap<QString, QString> s_sampleExtensions;

	static void loadExtensions( void );

} ;




#endif
