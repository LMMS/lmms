/*
 * ClapFxControls.cpp - ClapFxControls implementation
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

#include "ClapFxControls.h"

#include "ClapEffect.h"
#include "ClapFxControlDialog.h"
#include "Engine.h"

#include <QDomElement>

namespace lmms
{


ClapFxControls::ClapFxControls(ClapEffect* effect, const QString& uri)
	: EffectControls(effect), ClapControlBase(this, uri)
{
	if (isValid())
	{
		connect(Engine::audioEngine(), &AudioEngine::sampleRateChanged,
			this, [this](){ ClapControlBase::reload(); });
	}
}

void ClapFxControls::reload()
{
	ClapControlBase::reload();
	emit modelChanged();
}

void ClapFxControls::saveSettings(QDomDocument& doc, QDomElement& that)
{
	ClapControlBase::saveSettings(doc, that);
}

void ClapFxControls::loadSettings(const QDomElement& that)
{
	ClapControlBase::loadSettings(that);
}

auto ClapFxControls::controlCount() -> int
{
	return static_cast<int>(ClapControlBase::controlCount());
}

auto ClapFxControls::createView() -> gui::EffectControlDialog*
{
	return new gui::ClapFxControlDialog(this);
}

void ClapFxControls::changeControl()
{
	// TODO
	//	engine::getSong()->setModified();
}

auto ClapFxControls::settingsType() -> DataFile::Types
{
	return DataFile::EffectSettings;
}

void ClapFxControls::setNameFromFile(const QString& name)
{
	effect()->setDisplayName(name);
}


} // namespace lmms
