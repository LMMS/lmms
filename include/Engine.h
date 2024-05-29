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

#ifndef LMMS_ENGINE_H
#define LMMS_ENGINE_H

#include <QString>
#include <QObject>

#include "lmmsconfig.h"
#include "lmms_export.h"
#include "lmms_basics.h"

namespace lmms
{

class AudioEngine;
class Mixer;
class PatternStore;
class ProjectJournal;
class Song;
class Ladspa2LMMS;

namespace gui
{
class GuiApplication;
}


class LMMS_EXPORT Engine : public QObject
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

#ifdef LMMS_HAVE_CLAP
	static class ClapManager* getClapManager()
	{
		return s_clapManager;
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

	static inline Engine * inst()
	{
		if( s_instanceOfMe == nullptr )
		{
			s_instanceOfMe = new Engine();
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
	static inline void deleteHelper(T** ptr)
	{
		T* tmp = *ptr;
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
#ifdef LMMS_HAVE_CLAP
	static class ClapManager* s_clapManager;
#endif
	static Ladspa2LMMS* s_ladspaManager;
	static void* s_dndPluginKey;

	// even though most methods are static, an instance is needed for Qt slots/signals
	static Engine* s_instanceOfMe;

	friend class gui::GuiApplication;
};


} // namespace lmms

#endif // LMMS_ENGINE_H
