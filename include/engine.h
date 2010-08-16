/*
 * engine.h - engine-system of LMMS
 *
 * Copyright (c) 2006-2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

class AutomationEditor;
class AutomationRecorder;
class bbEditor;
class bbTrackContainer;
class DummyTrackContainer;
class FxMixer;
class FxMixerView;
class ProjectJournal;
class MainWindow;
class Mixer;
class pianoRoll;
class projectNotes;
class ResourceDB;
class song;
class songEditor;
class ladspa2LMMS;
class ControllerRackView;
class MidiControlListener;
class QDomDocument;


class EXPORT engine
{
public:
	static void init( const bool _has_gui = true );
	static void destroy();

	static bool hasGUI()
	{
		return s_hasGUI;
	}

	static void setSuppressMessages( bool _on )
	{
		s_suppressMessages = _on;
	}

	static bool suppressMessages()
	{
		return !s_hasGUI || s_suppressMessages;
	}

	// core
	static Mixer * getMixer()
	{
		return s_mixer;
	}

	static Mixer * mixer()
	{
		return s_mixer;
	}

	static FxMixer * fxMixer()
	{
		return s_fxMixer;
	}

	static song * getSong()
	{
		return s_song;
	}

	static bbTrackContainer * getBBTrackContainer()
	{
		return s_bbTrackContainer;
	}

	static ProjectJournal * projectJournal()
	{
		return s_projectJournal;
	}

	static ResourceDB * workingDirResourceDB()
	{
		return s_workingDirResourceDB;
	}

	static ResourceDB * webResourceDB()
	{
		return s_webResourceDB;
	}

	static ResourceDB * mergedResourceDB()
	{
		return s_mergedResourceDB;
	}

	// GUI
	static MainWindow * mainWindow()
	{
		return s_mainWindow;
	}

	static FxMixerView * fxMixerView()
	{
		return s_fxMixerView;
	}

	static songEditor * getSongEditor()
	{
		return s_songEditor;
	}

	static bbEditor * getBBEditor()
	{
		return s_bbEditor;
	}

	static pianoRoll * getPianoRoll()
	{
		return s_pianoRoll;
	}

	static projectNotes * getProjectNotes()
	{
		return s_projectNotes;
	}

	static AutomationEditor * automationEditor()
	{
		return s_automationEditor;
	}

	static AutomationRecorder * automationRecorder()
	{
		return s_automationRecorder;
	}

	static ladspa2LMMS * getLADSPAManager()
	{
		return s_ladspaManager;
	}

	static DummyTrackContainer * dummyTrackContainer()
	{
		return s_dummyTC;
	}

	static ControllerRackView * getControllerRackView()
	{
		return s_controllerRackView;
	}

	static float framesPerTick()
	{
		return s_framesPerTick;
	}
	static void updateFramesPerTick();

	static const QMap<QString, QString> & pluginFileHandling()
	{
		return s_pluginFileHandling;
	}

	static void setLmmsStyle( LmmsStyle * _style )
	{
		s_lmmsStyle = _style;
	}

	static LmmsStyle * getLmmsStyle()
	{
		return s_lmmsStyle;
	}
	
	static void saveConfiguration( QDomDocument & doc );

	static void loadConfiguration( QDomDocument & doc );

	static MidiControlListener * getMidiControlListener()
	{
		return s_midiControlListener;
	}

private:
	// small helper function which sets the pointer to NULL before actually deleting
	// the object it refers to
	template<class T>
	static inline void deleteHelper( T * * ptr )
	{
		T * tmp = *ptr;
		*ptr = NULL;
		delete tmp;
	}

	static bool s_hasGUI;
	static bool s_suppressMessages;
	static float s_framesPerTick;

	// core
	static Mixer * s_mixer;
	static FxMixer * s_fxMixer;
	static song * s_song;
	static ResourceDB * s_workingDirResourceDB;
	static ResourceDB * s_webResourceDB;
	static ResourceDB * s_mergedResourceDB;
	static bbTrackContainer * s_bbTrackContainer;
	static ProjectJournal * s_projectJournal;
	static DummyTrackContainer * s_dummyTC;
	static ControllerRackView * s_controllerRackView;
	static MidiControlListener * s_midiControlListener;

	// GUI
	static MainWindow * s_mainWindow;
	static FxMixerView * s_fxMixerView;
	static songEditor * s_songEditor;
	static AutomationEditor * s_automationEditor;
	static AutomationRecorder * s_automationRecorder;
	static bbEditor * s_bbEditor;
	static pianoRoll * s_pianoRoll;
	static projectNotes * s_projectNotes;
	static ladspa2LMMS * s_ladspaManager;

	static LmmsStyle * s_lmmsStyle;

	static QMap<QString, QString> s_pluginFileHandling;

	static void initPluginFileHandling();

} ;




#endif
