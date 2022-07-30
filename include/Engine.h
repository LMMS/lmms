/*
 * Engine.h - engine-system of LMMS
 *
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
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

#include <QString>
#include <QObject>


#include "lmmsconfig.h"
#include "lmms_export.h"
#include "lmms_basics.h"

class AudioEngine;
class Mixer;
class PatternStore;
class ProjectJournal;
class Song;
class Ladspa2LMMS;


// Note: This class is called 'LmmsCore' instead of 'Engine' because of naming
// conflicts caused by ZynAddSubFX. See https://github.com/LMMS/lmms/issues/2269
// and https://github.com/LMMS/lmms/pull/2118 for more details.
//
// The workaround was to rename Lmms' Engine so that it has a different symbol
// name in the object files, but typedef it back to 'Engine' and keep it inside
// of Engine.h so that the rest of the codebase can be oblivious to this issue
// (and it could be fixed without changing every single file).

class LmmsCore;
typedef LmmsCore Engine;

class LMMS_EXPORT LmmsCore : public QObject
{
	Q_OBJECT
public:
	static void init( bool renderOnly );
	static void destroy();

	// core
	static AudioEngine *audioEngine()
	{
		return s_audioEngine;
	}

	static Mixer * mixer()
	{
		return s_mixer;
	}

	static Song * getSong()
	{
		return s_song;
	}

	static PatternStore * patternStore()
	{
		return s_patternStore;
	}

	static ProjectJournal * projectJournal()
	{
		return s_projectJournal;
	}

	static bool ignorePluginBlacklist();

#ifdef LMMS_HAVE_LV2
	static class Lv2Manager * getLv2Manager()
	{
		return s_lv2Manager;
	}
#endif

	static Ladspa2LMMS * getLADSPAManager()
	{
		return s_ladspaManager;
	}

	static float framesPerTick()
	{
		return s_framesPerTick;
	}

	static float framesPerTick(sample_rate_t sample_rate);

	static void updateFramesPerTick();

	static inline LmmsCore * inst()
	{
		if( s_instanceOfMe == nullptr )
		{
			s_instanceOfMe = new LmmsCore();
		}
		return s_instanceOfMe;
	}

	static void setDndPluginKey(void* newKey);
	static void* pickDndPluginKey();

signals:
	void initProgress(const QString &msg);


private:
	// small helper function which sets the pointer to NULL before actually deleting
	// the object it refers to
	template<class T>
	static inline void deleteHelper( T * * ptr )
	{
		T * tmp = *ptr;
		*ptr = nullptr;
		delete tmp;
	}

	static float s_framesPerTick;

	// core
	static AudioEngine *s_audioEngine;
	static Mixer * s_mixer;
	static Song * s_song;
	static PatternStore * s_patternStore;
	static ProjectJournal * s_projectJournal;

#ifdef LMMS_HAVE_LV2
	static class Lv2Manager* s_lv2Manager;
#endif
	static Ladspa2LMMS * s_ladspaManager;
	static void* s_dndPluginKey;

	// even though most methods are static, an instance is needed for Qt slots/signals
	static LmmsCore * s_instanceOfMe;

	friend class GuiApplication;
};


#endif

