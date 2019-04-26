/*
 * SpaEffect.cpp - implementation of SPA effect
 *
 * Copyright (c) 2018-2019 Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>
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

#include "SpaEffect.h"

#include <QDebug>
#include <spa/spa.h>

#include "AutomatableModel.h"
#include "SpaFxControlDialog.h"
#include "SpaSubPluginFeatures.h"
#include "embed.h"
#include "plugin_export.h"

Plugin::Descriptor PLUGIN_EXPORT spaeffect_plugin_descriptor =
{
	STRINGIFY(PLUGIN_NAME),
	"SPA",
	QT_TRANSLATE_NOOP("SpaEffect",
		"plugin for using arbitrary SPA-effects inside LMMS."),
	"Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>",
	0x0100,
	Plugin::Effect,
	new PluginPixmapLoader("logo"),
	nullptr,
	new SpaSubPluginFeatures(Plugin::Effect)
};

SpaEffect::SpaEffect(Model* parent, const Descriptor::SubPluginFeatures::Key *key) :
	Effect(&spaeffect_plugin_descriptor, parent, key),
	m_controls(this, key->attributes["plugin"])
{
	qDebug() << "Constructing SPA effect";
}

SpaEffect::~SpaEffect()
{
}

bool SpaEffect::processAudioBuffer(sampleFrame *buf, const fpp_t frames)
{
	if(!isEnabled() || !isRunning())
	{
		return false;
	}

	SpaFxControls& ctrl = m_controls;

	ctrl.copyBuffersFromLmms(buf, frames);
	ctrl.copyModelsFromLmms();

//	m_pluginMutex.lock();
	ctrl.run(static_cast<unsigned>(frames));
//	m_pluginMutex.unlock();

	ctrl.copyBuffersToLmms(buf, frames);

	// TODO: check gate

	return isRunning();
}

unsigned SpaEffect::netPort(std::size_t chan) const
{
	return spaControls()->m_procs[chan]->netPort();
}

AutomatableModel *SpaEffect::modelAtPort(const QString &dest)
{
	return spaControls()->modelAtPort(dest);
}




extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin *lmms_plugin_main(Model *_parent, void *_data)
{
	using KeyType = Plugin::Descriptor::SubPluginFeatures::Key;
	return new SpaEffect(_parent, static_cast<const KeyType*>(_data));
}

}
