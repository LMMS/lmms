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
#include <QDebug>
#include <QtGlobal>
#include <QThread>
#include <QCoreApplication>

#include "Engine.h"
#include "ClapManager.h"
#include "ClapInstance.h"
#include "ClapTransport.h"

namespace lmms
{

ClapControlBase::ClapControlBase(Model* that, const QString& uri)
{
	qDebug() << "ClapControlBase::ClapControlBase";

	if (QThread::currentThread() == QCoreApplication::instance()->thread())
	{
		// On main thread - can start ClapInstance normally
		init(that, uri);
	}
	else
	{
		// Not on main thread - need to invoke on main thread
		static_assert(QT_VERSION >= QT_VERSION_CHECK(5, 10, 0), "");
		QMetaObject::invokeMethod(QCoreApplication::instance(), [&]{ init(that, uri); }, Qt::BlockingQueuedConnection);
	}
}

void ClapControlBase::init(Model* that, const QString& uri)
{
	auto manager = Engine::getClapManager();
	m_info = manager->pluginInfo(uri).lock().get();
	if (!m_info)
	{
		qCritical() << "No CLAP plugin found for URI" << uri;
		m_valid = false;
		return;
	}

	ClapTransport::update();

	qDebug() << "Creating CLAP instance (#1)";
	m_instances.clear();
	auto& first = m_instances.emplace_back(std::make_unique<ClapInstance>(m_info, that));
	if (!first || !first->isValid())
	{
		qCritical() << "Failed instantiating CLAP processor";
		m_instances.pop_back();
		m_valid = false;
		return;
	}

	if (!first->hasStereoOutput())
	{
		// A second instance is needed for stereo input/output
		qDebug() << "Creating CLAP instance (#2)";
		auto& second = m_instances.emplace_back(std::make_unique<ClapInstance>(m_info, that));
		if (!second || !second->isValid())
		{
			qCritical() << "Failed instantiating CLAP processor";
			m_instances.pop_back();
			m_valid = false;
			return;
		}
	}

	m_valid = true;
	m_channelsPerInstance = DEFAULT_CHANNELS / m_instances.size();
	setHasGui(first->gui() != nullptr);
	linkAllModels();
}

ClapControlBase::~ClapControlBase() = default;

auto ClapControlBase::getGroup(std::size_t idx) -> LinkedModelGroup*
{
	return (idx < m_instances.size()) ? m_instances[idx].get() : nullptr;
}

auto ClapControlBase::getGroup(std::size_t idx) const -> const LinkedModelGroup*
{
	return (idx < m_instances.size()) ? m_instances[idx].get() : nullptr;
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
		instance->copyBuffersFromCore(buf, firstChan, m_channelsPerInstance, frames);
		firstChan += m_channelsPerInstance;
	}
}

void ClapControlBase::copyBuffersToLmms(sampleFrame* buf, fpp_t frames) const
{
	unsigned firstChan = 0; // tell the instances which channels they shall write to
	for (const auto& instance : m_instances)
	{
		instance->copyBuffersToCore(buf, firstChan, m_channelsPerInstance, frames);
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
	qDebug() << "ClapControlBase::loadFile called, but it is unimplemented.";
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

void ClapControlBase::callHostIdle()
{
	for (const auto& instance : m_instances)
	{
		instance->hostIdle();
	}
}

} // namespace lmms

#endif // LMMS_HAVE_CLAP
