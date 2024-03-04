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
#include "AudioEngine.h"
#include "ConfigManager.h"
#include "Mixer.h"
#include "Ladspa2LMMS.h"
#include "Lv2Manager.h"
#include "PatternStore.h"
#include "Plugin.h"
#include "PresetPreviewPlayHandle.h"
#include "ProjectJournal.h"
#include "Song.h"
#include "BandLimitedWave.h"
#include "Oscillator.h"

namespace lmms
{

float Engine::s_framesPerTick;
std::unique_ptr<AudioEngine> Engine::s_audioEngine = nullptr;
std::unique_ptr<Mixer> Engine::s_mixer = nullptr;
std::unique_ptr<PatternStore> Engine::s_patternStore = nullptr;
std::unique_ptr<Song> Engine::s_song = nullptr;
std::unique_ptr<ProjectJournal> Engine::s_projectJournal = nullptr;
#ifdef LMMS_HAVE_LV2
std::unique_ptr<Lv2Manager> Engine::s_lv2Manager = nullptr;
#endif
std::unique_ptr<Ladspa2LMMS> Engine::s_ladspaManager = nullptr;
void* Engine::s_dndPluginKey = nullptr;




void Engine::init( bool renderOnly )
{
	Engine *engine = inst();

	emit engine->initProgress(tr("Generating wavetables"));
	// generate (load from file) bandlimited wavetables
	BandLimitedWave::generateWaves();
	//initilize oscillators
	Oscillator::waveTableInit();

	emit engine->initProgress(tr("Initializing data structures"));
	s_projectJournal = std::make_unique<ProjectJournal>();
	s_audioEngine = std::make_unique<AudioEngine>(renderOnly);
	s_song = std::make_unique<Song>();
	s_mixer = std::make_unique<Mixer>();
	s_patternStore = std::make_unique<PatternStore>();

#ifdef LMMS_HAVE_LV2
	s_lv2Manager = std::make_unique<Lv2Manager>();
	s_lv2Manager->initPlugins();
#endif
	s_ladspaManager = std::make_unique<Ladspa2LMMS>();

	s_projectJournal->setJournalling( true );

	emit engine->initProgress(tr("Opening audio and midi devices"));
	s_audioEngine->initDevices();

	PresetPreviewPlayHandle::init();

	emit engine->initProgress(tr("Launching audio engine threads"));
	s_audioEngine->startProcessing();
}




void Engine::destroy()
{
	s_projectJournal->stopAllJournalling();
	s_audioEngine->stopProcessing();

	PresetPreviewPlayHandle::cleanup();

	s_song->clearProject();

	delete ConfigManager::inst();

	// The oscillator FFT plans remain throughout the application lifecycle
	// due to being expensive to create, and being used whenever a userwave form is changed
	Oscillator::destroyFFTPlans();
}




bool Engine::ignorePluginBlacklist()
{
	const char* envVar = getenv("LMMS_IGNORE_BLACKLIST");
	return (envVar && *envVar);
}




float Engine::framesPerTick(sample_rate_t sampleRate)
{
	return sampleRate * 60.0f * 4 /
			DefaultTicksPerBar / s_song->getTempo();
}




void Engine::updateFramesPerTick()
{
	s_framesPerTick = s_audioEngine->processingSampleRate() * 60.0f * 4 / DefaultTicksPerBar / s_song->getTempo();
}




void Engine::setDndPluginKey(void *newKey)
{
	Q_ASSERT(static_cast<Plugin::Descriptor::SubPluginFeatures::Key*>(newKey));
	s_dndPluginKey = newKey;
}




void *Engine::pickDndPluginKey()
{
	return s_dndPluginKey;
}




std::unique_ptr<Engine> Engine::s_instanceOfMe = nullptr;

} // namespace lmms