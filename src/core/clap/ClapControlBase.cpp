/*
 * ClapControlBase.cpp - CLAP control base class
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

#include "ClapControlBase.h"

#ifdef LMMS_HAVE_CLAP

#include <algorithm>
#include <cassert>

#include "ClapManager.h"
#include "ClapInstance.h"
#include "ClapTransport.h"
#include "Engine.h"

namespace lmms
{

ClapControlBase::ClapControlBase(Model* that, const QString& uri)
{
	init(that, uri);
}

void ClapControlBase::init(Model* that, const QString& uri)
{
	// CLAP API requires main thread for plugin loading
	assert(ClapThreadCheck::isMainThread());

	m_valid = false;
	auto manager = Engine::getClapManager();
	m_info = manager->pluginInfo(uri).lock().get();
	if (!m_info)
	{
		qCritical() << "No CLAP plugin found for URI" << uri;
		return;
	}

	ClapTransport::update();

	ClapLog::globalLog(CLAP_LOG_INFO, "Creating CLAP instance (#1)");
	m_instances.clear();
	auto& first = m_instances.emplace_back(std::make_unique<ClapInstance>(m_info, that));
	if (!first || !first->isValid())
	{
		ClapLog::globalLog(CLAP_LOG_ERROR, "Failed instantiating CLAP processor");
		m_instances.clear();
		return;
	}

	if (!first->audioPorts().hasStereoOutput())
	{
		// A second instance is needed for stereo input/output
		ClapLog::globalLog(CLAP_LOG_INFO, "Creating CLAP instance (#2)");
		auto& second = m_instances.emplace_back(std::make_unique<ClapInstance>(m_info, that));
		if (!second || !second->isValid())
		{
			ClapLog::globalLog(CLAP_LOG_ERROR, "Failed instantiating CLAP processor");
			m_instances.clear();
			return;
		}
	}

	m_channelsPerInstance = DEFAULT_CHANNELS / m_instances.size();
	m_valid = true;

	linkAllModels();
}

auto ClapControlBase::getGroup(std::size_t idx) -> LinkedModelGroup*
{
	return (idx < m_instances.size()) ? &m_instances[idx]->params() : nullptr;
}

auto ClapControlBase::getGroup(std::size_t idx) const -> const LinkedModelGroup*
{
	return (idx < m_instances.size()) ? &m_instances[idx]->params() : nullptr;
}

void ClapControlBase::copyModelsFromLmms()
{
	for (const auto& instance : m_instances)
	{
		instance->copyModelsFromCore();
	}
}

void ClapControlBase::copyModelsToLmms() const
{
	for (const auto& instance : m_instances)
	{
		instance->copyModelsToCore();
	}
}

void ClapControlBase::copyBuffersFromLmms(const sampleFrame* buf, fpp_t frames)
{
	unsigned firstChan = 0; // tell the instances which channels they shall read from
	for (const auto& instance : m_instances) 
	{
		instance->audioPorts().copyBuffersFromCore(buf, firstChan, m_channelsPerInstance, frames);
		firstChan += m_channelsPerInstance;
	}
}

void ClapControlBase::copyBuffersToLmms(sampleFrame* buf, fpp_t frames) const
{
	unsigned firstChan = 0; // tell the instances which channels they shall write to
	for (const auto& instance : m_instances)
	{
		instance->audioPorts().copyBuffersToCore(buf, firstChan, m_channelsPerInstance, frames);
		firstChan += m_channelsPerInstance;
	}
}

void ClapControlBase::run(fpp_t frames)
{
	for (const auto& instance : m_instances)
	{
		instance->run(frames);
	}
}

void ClapControlBase::saveSettings(QDomDocument& doc, QDomElement& elem)
{
	LinkedModelGroups::saveSettings(doc, elem);

	// TODO: save state using clap_plugin_state if supported by plugin
}

void ClapControlBase::loadSettings(const QDomElement& elem)
{
	LinkedModelGroups::loadSettings(elem);

	// TODO: load state using clap_plugin_state if supported by plugin
}

void ClapControlBase::loadFile([[maybe_unused]] const QString& file)
{
	// TODO: load preset using clap_plugin_preset_load if supported by plugin
	ClapLog::globalLog(CLAP_LOG_ERROR, "ClapControlBase::loadFile() called, but it is unimplemented");
}

void ClapControlBase::reload()
{
	for (const auto& instance : m_instances)
	{
		instance->restart();
	}
}

auto ClapControlBase::controlCount() const -> std::size_t
{
	std::size_t res = 0;
	for (const auto& instance : m_instances)
	{
		res += instance->controlCount();
	}
	return res;
}

bool ClapControlBase::hasNoteInput() const
{
	return std::any_of(m_instances.begin(), m_instances.end(),
		[](const auto& instance) { return instance->hasNoteInput(); });
}

void ClapControlBase::handleMidiInputEvent(const MidiEvent& event, const TimePos& time, f_cnt_t offset)
{
	for (const auto& instance : m_instances)
	{
		instance->handleMidiInputEvent(event, time, offset);
	}
}

void ClapControlBase::idle()
{
	for (const auto& instance : m_instances)
	{
		instance->idle();
	}
}

} // namespace lmms

#endif // LMMS_HAVE_CLAP
