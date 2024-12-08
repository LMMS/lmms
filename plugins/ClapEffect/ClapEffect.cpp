/*
 * ClapEffect.cpp - Implementation of CLAP effect
 *
 * Copyright (c) 2024 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#include <QApplication>
#include <memory>

#include "ClapInstance.h"
#include "ClapSubPluginFeatures.h"
#include "embed.h"
#include "plugin_export.h"

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

PLUGIN_EXPORT Plugin* lmms_plugin_main(Model* parent, void* data)
{
	using KeyType = Plugin::Descriptor::SubPluginFeatures::Key;
	auto effect = std::make_unique<ClapEffect>(parent, static_cast<const KeyType*>(data));
	if (!effect || !effect->isValid()) { return nullptr; }
	return effect.release();
}

} // extern "C"


ClapEffect::ClapEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key)
	: Effect{&clapeffect_plugin_descriptor, parent, key}
	, m_controls{this, key->attributes["uri"].toStdString()}
	, m_tempOutputSamples(Engine::audioEngine()->framesPerPeriod())
{
	connect(Engine::audioEngine(), &AudioEngine::sampleRateChanged, this,
		[&] { m_tempOutputSamples.resize(Engine::audioEngine()->framesPerPeriod()); });
}

auto ClapEffect::processImpl(SampleFrame* buf, const fpp_t frames) -> ProcessStatus
{
	ClapInstance* instance = m_controls.m_instance.get();
	assert(instance != nullptr);
	assert(frames <= static_cast<fpp_t>(m_tempOutputSamples.size()));

	instance->audioPorts().copyBuffersFromCore(buf, frames);
	instance->copyModelsFromCore();

	instance->run(frames);

	instance->copyModelsToCore();
	instance->audioPorts().copyBuffersToCore(m_tempOutputSamples.data(), frames);

	SampleFrame* leftSamples = m_tempOutputSamples.data();
	SampleFrame* rightSamples = m_tempOutputSamples.data();
	switch (instance->audioPorts().portConfig<false>())
	{
		case PluginPortConfig::Config::LeftOnly:
			rightSamples = buf; break;
		case PluginPortConfig::Config::RightOnly:
			leftSamples = buf; break;
		default: break;
	}

	bool corrupt = wetLevel() < 0.f; // #3261 - if wet < 0, bash wet := 0, dry := 1
	const float dry = corrupt ? 1.f : dryLevel();
	const float wet = corrupt ? 0.f : wetLevel();

	for (fpp_t f = 0; f < frames; ++f)
	{
		buf[f][0] = dry * buf[f][0] + wet * leftSamples[f][0];
		buf[f][1] = dry * buf[f][1] + wet * rightSamples[f][1];
	}

	return ProcessStatus::ContinueIfNotQuiet;
}

} // namespace lmms
