/*
 * ClapEffect.cpp - implementation of CLAP effect
 *
 * Copyright (c) 2023 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#include "ClapEffect.h"

#include "ClapSubPluginFeatures.h"

#include "embed.h"
#include "plugin_export.h"

#include <memory>

namespace lmms
{


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT clapeffect_plugin_descriptor =
{
	LMMS_STRINGIFY(PLUGIN_NAME),
	"CLAP",
	QT_TRANSLATE_NOOP("PluginBrowser",
		"plugin for using CLAP effects inside LMMS."),
	"Dalton Messmer <messmer.dalton/at/gmail.com>",
	0x0100,
	Plugin::Type::Effect,
	new PluginPixmapLoader("logo"),
	nullptr,
	new ClapSubPluginFeatures(Plugin::Type::Effect)
};

}


ClapEffect::ClapEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key)
	: Effect(&clapeffect_plugin_descriptor, parent, key),
	m_controls(this, key->attributes["uri"]),
	m_tempOutputSamples(Engine::audioEngine()->framesPerPeriod()),
	m_idleTimer(this)
{
	connect(&m_idleTimer, &QTimer::timeout, this, QOverload<>::of(&ClapEffect::callHostIdle));
	m_idleTimer.start(1000 / 30);
}

bool ClapEffect::processAudioBuffer(sampleFrame* buf, const fpp_t frames)
{
	if (!isEnabled() || !isRunning()) { return false; }
	Q_ASSERT(frames <= static_cast<fpp_t>(m_tempOutputSamples.size()));

	m_controls.copyBuffersFromLmms(buf, frames);
	m_controls.copyModelsFromLmms();

//	m_pluginMutex.lock();
	m_controls.run(frames);
//	m_pluginMutex.unlock();

	m_controls.copyModelsToLmms();
	m_controls.copyBuffersToLmms(m_tempOutputSamples.data(), frames);

	double outSum = 0.0;
	bool corrupt = wetLevel() < 0.f; // #3261 - if w < 0, bash w := 0, d := 1
	const float d = corrupt ? 1.f : dryLevel();
	const float w = corrupt ? 0.f : wetLevel();
	for (fpp_t f = 0; f < frames; ++f)
	{
		buf[f][0] = d * buf[f][0] + w * m_tempOutputSamples[f][0];
		buf[f][1] = d * buf[f][1] + w * m_tempOutputSamples[f][1];
		auto l = static_cast<double>(buf[f][0]);
		auto r = static_cast<double>(buf[f][1]);
		outSum += l*l + r*r;
	}
	checkGate(outSum / frames);

	return isRunning();
}


extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin* lmms_plugin_main(Model* parent, void* data)
{
	using KeyType = Plugin::Descriptor::SubPluginFeatures::Key;
	auto effect = std::make_unique<ClapEffect>(parent, static_cast<const KeyType*>(data));
	if (!effect || !effect->isValid())
		return nullptr;
	return effect.release();
}

}


} // namespace lmms
