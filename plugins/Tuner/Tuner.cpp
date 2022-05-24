/*
 * Tuner.cpp - determine the fundamental frequency of audio signals
 *
 * Copyright (c) 2022 saker <sakertooth@gmail.com>
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

#include "Tuner.h"

#include "embed.h"
#include "plugin_export.h"

extern "C" {

Plugin::Descriptor PLUGIN_EXPORT tuner_plugin_descriptor = {STRINGIFY(PLUGIN_NAME), "Tuner",
	QT_TRANSLATE_NOOP("pluginBrowser", "Determine the fundamental frequency of audio signals"),
	"saker <sakertooth@gmail.com>", 0x0100, Plugin::Effect, new PluginPixmapLoader("logo"), NULL, NULL};
};

extern "C" {
Plugin* PLUGIN_EXPORT lmms_plugin_main(Model* parent, void* _data)
{
	return new Tuner(parent, static_cast<const Plugin::Descriptor::SubPluginFeatures::Key *>(_data));
}
}

Tuner::Tuner(Model* parent, const Descriptor::SubPluginFeatures::Key* key)
	: Effect(&tuner_plugin_descriptor, parent, key)
	, m_tunerControls(this)
{
}

bool Tuner::processAudioBuffer(sampleFrame* buf, const fpp_t frames) 
{

}

EffectControls* Tuner::controls() 
{
	return new TunerControls(this);
}