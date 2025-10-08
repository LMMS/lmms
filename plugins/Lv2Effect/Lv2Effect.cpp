/*
 * Lv2Effect.cpp - implementation of LV2 effect
 *
 * Copyright (c) 2018-2023 Johannes Lorenz <jlsf2013$users.sourceforge.net, $=@>
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

#include "Lv2Effect.h"

#include <QDebug>

#include "Lv2SubPluginFeatures.h"

#include "embed.h"
#include "plugin_export.h"


namespace lmms
{


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT lv2effect_plugin_descriptor =
{
	LMMS_STRINGIFY(PLUGIN_NAME),
	"LV2",
	QT_TRANSLATE_NOOP("PluginBrowser",
		"plugin for using arbitrary LV2-effects inside LMMS."),
	"Johannes Lorenz <jlsf2013$$$users.sourceforge.net, $$$=@>",
	0x0100,
	Plugin::Type::Effect,
	new PluginPixmapLoader("logo"),
	{},
	new Lv2SubPluginFeatures(Plugin::Type::Effect)
};

}




Lv2Effect::Lv2Effect(Model* parent, const Descriptor::SubPluginFeatures::Key *key) :
	Effect(&lv2effect_plugin_descriptor, parent, key),
	m_controls(this, key->attributes["uri"]),
	m_tmpOutputSmps(Engine::audioEngine()->framesPerPeriod())
{
}




Effect::ProcessStatus Lv2Effect::processImpl(SampleFrame* buf, const fpp_t frames)
{
	Q_ASSERT(frames <= static_cast<fpp_t>(m_tmpOutputSmps.size()));

	m_controls.copyBuffersFromLmms(buf, frames);
	m_controls.copyModelsFromLmms();

//	m_pluginMutex.lock();
	m_controls.run(frames);
//	m_pluginMutex.unlock();

	m_controls.copyModelsToLmms();
	m_controls.copyBuffersToLmms(m_tmpOutputSmps.data(), frames);

	bool corrupt = wetLevel() < 0; // #3261 - if w < 0, bash w := 0, d := 1
	const float d = corrupt ? 1 : dryLevel();
	const float w = corrupt ? 0 : wetLevel();
	for(fpp_t f = 0; f < frames; ++f)
	{
		buf[f][0] = d * buf[f][0] + w * m_tmpOutputSmps[f][0];
		buf[f][1] = d * buf[f][1] + w * m_tmpOutputSmps[f][1];
	}

	return ProcessStatus::ContinueIfNotQuiet;
}




extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin *lmms_plugin_main(Model *_parent, void *_data)
{
	using KeyType = Plugin::Descriptor::SubPluginFeatures::Key;
	try {
		return new Lv2Effect(_parent, static_cast<const KeyType*>(_data));
	} catch (const std::runtime_error& e) {
		qCritical() << e.what();
		return nullptr;
	}
}

}


} // namespace lmms
