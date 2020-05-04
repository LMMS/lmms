/*
 * Engine.cpp - implementation of LMMS' engine-system
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


#include "Engine.h"
#include "BBTrackContainer.h"
#include "ConfigManager.h"
#include "FxMixer.h"
#include "Ladspa2LMMS.h"
#include "Mixer.h"
#include "Plugin.h"
#include "PresetPreviewPlayHandle.h"
#include "ProjectJournal.h"
#include "Song.h"
#include "BandLimitedWave.h"

float LmmsCore::s_framesPerTick;
Mixer* LmmsCore::s_mixer = NULL;
FxMixer * LmmsCore::s_fxMixer = NULL;
BBTrackContainer * LmmsCore::s_bbTrackContainer = NULL;
Song * LmmsCore::s_song = NULL;
ProjectJournal * LmmsCore::s_projectJournal = NULL;
Ladspa2LMMS * LmmsCore::s_ladspaManager = NULL;
void* LmmsCore::s_dndPluginKey = nullptr;
DummyTrackContainer * LmmsCore::s_dummyTC = NULL;




void LmmsCore::init( bool renderOnly )
{
	LmmsCore *engine = inst();

	emit engine->initProgress(tr("Generating wavetables"));
	// generate (load from file) bandlimited wavetables
	BandLimitedWave::generateWaves();

	emit engine->initProgress(tr("Initializing data structures"));
	s_projectJournal = new ProjectJournal;
	s_mixer = new Mixer( renderOnly );
	s_song = new Song;
	s_fxMixer = new FxMixer;
	s_bbTrackContainer = new BBTrackContainer;

	s_ladspaManager = new Ladspa2LMMS;

	s_projectJournal->setJournalling( true );

	emit engine->initProgress(tr("Opening audio and midi devices"));
	s_mixer->initDevices();

	PresetPreviewPlayHandle::init();
	s_dummyTC = new DummyTrackContainer;

	emit engine->initProgress(tr("Launching mixer threads"));
	s_mixer->startProcessing();
}




void LmmsCore::destroy()
{
	s_projectJournal->stopAllJournalling();
	s_mixer->stopProcessing();

	PresetPreviewPlayHandle::cleanup();

	s_song->clearProject();

	deleteHelper( &s_bbTrackContainer );
	deleteHelper( &s_dummyTC );

	deleteHelper( &s_fxMixer );
	deleteHelper( &s_mixer );

	deleteHelper( &s_ladspaManager );

	//delete ConfigManager::inst();
	deleteHelper( &s_projectJournal );

	deleteHelper( &s_song );

	delete ConfigManager::inst();
}

float LmmsCore::framesPerTick(sample_rate_t sampleRate)
{
	return sampleRate * 60.0f * 4 /
			DefaultTicksPerBar / s_song->getTempo();
}




void LmmsCore::updateFramesPerTick()
{
	s_framesPerTick = s_mixer->processingSampleRate() * 60.0f * 4 /
				DefaultTicksPerBar / s_song->getTempo();
}




void LmmsCore::setDndPluginKey(void *newKey)
{
	Q_ASSERT(static_cast<Plugin::Descriptor::SubPluginFeatures::Key*>(newKey));
	s_dndPluginKey = newKey;
}




void *LmmsCore::pickDndPluginKey()
{
	return s_dndPluginKey;
}




LmmsCore * LmmsCore::s_instanceOfMe = NULL;
