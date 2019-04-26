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


#include <QUrl>

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
#include "SpaManager.h"
#include "BandLimitedWave.h"

float LmmsCore::s_framesPerTick;
Mixer* LmmsCore::s_mixer = NULL;
FxMixer * LmmsCore::s_fxMixer = NULL;
BBTrackContainer * LmmsCore::s_bbTrackContainer = NULL;
Song * LmmsCore::s_song = NULL;
ProjectJournal * LmmsCore::s_projectJournal = NULL;
Ladspa2LMMS * LmmsCore::s_ladspaManager = NULL;
#ifdef LMMS_HAVE_SPA
SpaManager * LmmsCore::s_spaManager = nullptr;
#endif
void* LmmsCore::s_dndPluginKey = nullptr;
QMap<unsigned, class Plugin*> LmmsCore::s_pluginsByPort;
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
#ifdef LMMS_HAVE_SPA
	s_spaManager = new SpaManager;
#endif
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
#ifdef LMMS_HAVE_SPA
	deleteHelper( &s_spaManager );
#endif

	//delete ConfigManager::inst();
	deleteHelper( &s_projectJournal );

	deleteHelper( &s_song );

	delete ConfigManager::inst();
}




void LmmsCore::updateFramesPerTick()
{
	s_framesPerTick = s_mixer->processingSampleRate() * 60.0f * 4 /
				DefaultTicksPerTact / s_song->getTempo();
}




// url: val as url
AutomatableModel *LmmsCore::getAutomatableModelAtPort(const QString& val,
	const QUrl& url)
{
	AutomatableModel* mod = nullptr;

	// qDebug() << val;
	auto itr = s_pluginsByPort.find(static_cast<unsigned>(url.port()));
	if(itr == s_pluginsByPort.end())
	{
		puts(	"DnD from a plugin which is not "
			"in LMMS... ignoring");
		// TODO: MessageBox?
	}
	else
	{
		mod = itr.value()->modelAtPort(val);
	}

	return mod;
}




AutomatableModel *LmmsCore::getAutomatableModel(const QString& val, bool hasPort)
{
	AutomatableModel* mod = nullptr;
	if(hasPort)
	{
		QUrl url(val);
		if(!url.isValid())
		{
			printf( "Could not find a port in %s => "
				"can not make connection\n",
				val.toUtf8().data());
		}
		else
		{
			mod = getAutomatableModelAtPort(val, url);
		}
	}
	else
	{
		mod = dynamic_cast<AutomatableModel *>(
			projectJournal()->
			journallingObject( val.toInt() ) );
	}
	return mod;
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




void LmmsCore::addPluginByPort(unsigned port, Plugin *plug)
{
	s_pluginsByPort.insert(port, plug);
}




LmmsCore * LmmsCore::s_instanceOfMe = NULL;
