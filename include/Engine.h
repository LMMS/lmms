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

#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QObject>


#include "export.h"

class BBTrackContainer;
class DummyTrackContainer;
class FxMixer;
class ProjectJournal;
class Mixer;
class Song;
class Ladspa2LMMS;


class EXPORT Engine : public QObject
{
	Q_OBJECT
public:
	static void init();
	static void destroy();

	// TODO: Remove me. Replace calls like `if( Engine::hasGUI() )` with
	// `if (gui)` (gui defined in "GuiApplication.h"
	static bool hasGUI();

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

	static Ladspa2LMMS * getLADSPAManager()
	{
		return s_ladspaManager;
	}

	static DummyTrackContainer * dummyTrackContainer()
	{
		return s_dummyTC;
	}

	static float framesPerTick()
	{
		return s_framesPerTick;
	}
	static void updateFramesPerTick();

	static inline Engine * inst()
	{
		if( s_instanceOfMe == NULL )
		{
			s_instanceOfMe = new Engine();
		}
		return s_instanceOfMe;
	}

signals:
	void initProgress(const QString &msg);


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

	static float s_framesPerTick;

	// core
	static Mixer *s_mixer;
	static FxMixer * s_fxMixer;
	static Song * s_song;
	static BBTrackContainer * s_bbTrackContainer;
	static ProjectJournal * s_projectJournal;
	static DummyTrackContainer * s_dummyTC;

	static Ladspa2LMMS * s_ladspaManager;

	// even though most methods are static, an instance is needed for Qt slots/signals
	static Engine * s_instanceOfMe;

	friend class GuiApplication;
};




#endif
