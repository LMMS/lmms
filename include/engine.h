/*
 * engine.h - engine-system of LMMS
 *
 * Copyright (c) 2006-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "lmmsconfig.h"

#include <QtCore/QMap>

#include "export.h"
#include "lmms_style.h"

class automationEditor;
class AutomationRecorder;
class bbEditor;
class bbTrackContainer;
class dummyTrackContainer;
class fxMixer;
class fxMixerView;
class projectJournal;
class mainWindow;
class mixer;
class pianoRoll;
class projectNotes;
class UnifiedResourceProvider;
class song;
class songEditor;
class ladspa2LMMS;
class controllerRackView;
class MidiControlListener;
class QDomDocument;


class EXPORT engine
{
public:
	static void init( const bool _has_gui = true );
	static void destroy( void );

	static bool hasGUI( void )
	{
		return s_hasGUI;
	}

	static void setSuppressMessages( bool _on )
	{
		s_suppressMessages = _on;
	}

	static bool suppressMessages( void )
	{
		return !s_hasGUI || s_suppressMessages;
	}

	// core
	static mixer * getMixer( void )
	{
		return s_mixer;
	}

	static fxMixer * getFxMixer( void )
	{
		return s_fxMixer;
	}

	static song * getSong( void )
	{
		return s_song;
	}

	static bbTrackContainer * getBBTrackContainer( void )
	{
		return s_bbTrackContainer;
	}

	static projectJournal * getProjectJournal( void )
	{
		return s_projectJournal;
	}

	static UnifiedResourceProvider * resourceProvider( void )
	{
		return s_resourceProvider;
	}

	// GUI
	static mainWindow * getMainWindow( void )
	{
		return s_mainWindow;
	}

	static fxMixerView * getFxMixerView( void )
	{
		return s_fxMixerView;
	}

	static songEditor * getSongEditor( void )
	{
		return s_songEditor;
	}

	static bbEditor * getBBEditor( void )
	{
		return s_bbEditor;
	}

	static pianoRoll * getPianoRoll( void )
	{
		return s_pianoRoll;
	}

	static projectNotes * getProjectNotes( void )
	{
		return s_projectNotes;
	}

	static automationEditor * getAutomationEditor( void )
	{
		return s_automationEditor;
	}

	static AutomationRecorder * getAutomationRecorder( void )
	{
		return s_automationRecorder;
	}

	static ladspa2LMMS * getLADSPAManager( void )
	{
		return s_ladspaManager;
	}

	static dummyTrackContainer * getDummyTrackContainer( void )
	{
		return s_dummyTC;
	}

	static controllerRackView * getControllerRackView( void )
	{
		return s_controllerRackView;
	}

	static float framesPerTick( void )
	{
		return s_framesPerTick;
	}
	static void updateFramesPerTick( void );

	static const QMap<QString, QString> & pluginFileHandling( void )
	{
		return s_pluginFileHandling;
	}

	static void setLmmsStyle( LmmsStyle * _style )
	{
		s_lmmsStyle = _style;
	}

	static LmmsStyle * getLmmsStyle( void )
	{
		return s_lmmsStyle;
	}
	
	static void saveConfiguration( QDomDocument & doc );

	static void loadConfiguration( QDomDocument & doc );

	static MidiControlListener * getMidiControlListener( void )
	{
		return s_midiControlListener;
	}

private:
	static bool s_hasGUI;
	static bool s_suppressMessages;
	static float s_framesPerTick;

	// core
	static mixer * s_mixer;
	static fxMixer * s_fxMixer;
	static song * s_song;
	static UnifiedResourceProvider * s_resourceProvider;
	static bbTrackContainer * s_bbTrackContainer;
	static projectJournal * s_projectJournal;
	static dummyTrackContainer * s_dummyTC;
	static controllerRackView * s_controllerRackView;
	static MidiControlListener * s_midiControlListener;

	// GUI
	static mainWindow * s_mainWindow;
	static fxMixerView * s_fxMixerView;
	static songEditor * s_songEditor;
	static automationEditor * s_automationEditor;
	static AutomationRecorder * s_automationRecorder;
	static bbEditor * s_bbEditor;
	static pianoRoll * s_pianoRoll;
	static projectNotes * s_projectNotes;
	static ladspa2LMMS * s_ladspaManager;

	static LmmsStyle * s_lmmsStyle;

	static QMap<QString, QString> s_pluginFileHandling;

	static void initPluginFileHandling( void );

} ;




#endif
