/*
 * ClapFxControls.cpp - ClapFxControls implementation
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

#include "ClapFxControls.h"

#include <QDomElement>

#include "ClapEffect.h"
#include "ClapFxControlDialog.h"
#include "ClapInstance.h"
#include "Engine.h"

namespace lmms
{

ClapFxControls::ClapFxControls(ClapEffect* effect, const std::string& pluginId)
	: EffectControls{effect}
	, m_instance{ClapInstance::create(pluginId, this)}
	, m_idleTimer{this}
{
	if (isValid())
	{
		connect(Engine::audioEngine(), &AudioEngine::sampleRateChanged, this, &ClapFxControls::reload);
		connect(&m_idleTimer, &QTimer::timeout, m_instance.get(), &ClapInstance::idle);
		m_idleTimer.start(1000 / 30);
	}
}

auto ClapFxControls::isValid() const -> bool
{
	return m_instance != nullptr
		&& m_instance->isValid();
}

void ClapFxControls::reload()
{
	if (m_instance) { m_instance->restart(); }

	emit modelChanged();
}

void ClapFxControls::saveSettings(QDomDocument& doc, QDomElement& elem)
{
	if (!m_instance) { return; }
	m_instance->saveSettings(doc, elem);
}

void ClapFxControls::loadSettings(const QDomElement& elem)
{
	if (!m_instance) { return; }
	m_instance->loadSettings(elem);
}

auto ClapFxControls::controlCount() -> int
{
	return m_instance ? m_instance->controlCount() : 0;
}

auto ClapFxControls::createView() -> gui::EffectControlDialog*
{
	return new gui::ClapFxControlDialog{this};
}

void ClapFxControls::changeControl()
{
	// TODO
	//	engine::getSong()->setModified();
}

} // namespace lmms
