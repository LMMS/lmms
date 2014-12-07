/*
 * Engine.h - engine-system of LMMS
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


#ifndef ENGINE_H
#define ENGINE_H

#include "lmmsconfig.h"
#include "MemoryManager.h"

#include <QtCore/QMap>

#include "export.h"

class AutomationEditorWindow;
class BBEditor;
class BBTrackContainer;
class DummyTrackContainer;
class FxMixer;
class FxMixerView;
class ProjectJournal;
class MainWindow;
class Mixer;
class PianoRoll;
class ProjectNotes;
class Song;
class SongEditor;
class Ladspa2LMMS;
class ControllerRackView;


class EXPORT Engine
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
	static Mixer *mixer()
	{
		return s_mixer;
	}

	static FxMixer * fxMixer()
	{
		return s_fxMixer;
	}

	static Song * getSong()
	{
		return s_song;
	}

	static BBTrackContainer * getBBTrackContainer()
	{
		return s_bbTrackContainer;
	}

	static ProjectJournal * projectJournal()
	{
		return s_projectJournal;
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

	static SongEditor* songEditor()
	{
		return s_songEditor;
	}

	static BBEditor * getBBEditor()
	{
		return s_bbEditor;
	}

	static PianoRoll* pianoRoll()
	{
		return s_pianoRoll;
	}

	static ProjectNotes * getProjectNotes()
	{
		return s_projectNotes;
	}

	static AutomationEditorWindow * automationEditor()
	{
		return s_automationEditor;
	}

	static Ladspa2LMMS * getLADSPAManager()
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
	static Mixer *s_mixer;
	static FxMixer * s_fxMixer;
	static Song * s_song;
	static BBTrackContainer * s_bbTrackContainer;
	static ProjectJournal * s_projectJournal;
	static DummyTrackContainer * s_dummyTC;
	static ControllerRackView * s_controllerRackView;

	// GUI
	static MainWindow * s_mainWindow;
	static FxMixerView * s_fxMixerView;
	static SongEditor* s_songEditor;
	static AutomationEditorWindow * s_automationEditor;
	static BBEditor * s_bbEditor;
	static PianoRoll* s_pianoRoll;
	static ProjectNotes * s_projectNotes;
	static Ladspa2LMMS * s_ladspaManager;

	static QMap<QString, QString> s_pluginFileHandling;

	static void initPluginFileHandling();

} ;




#endif
