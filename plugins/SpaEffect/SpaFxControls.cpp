/*
 * SpaControls.cpp - controls for amplifier effect
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

#include "SpaFxControls.h"

#include "Engine.h"
#include "SpaEffect.h"
#include "SpaFxControlDialog.h"

SpaFxControls::SpaFxControls(class SpaEffect *effect, const QString& uniqueName) :
	EffectControls(effect),
	SpaControlBase(static_cast<EffectControls*>(this), uniqueName,
		DataFile::Type::EffectSettings),
	m_effect(effect)
{
	if (isValid())
	{
		connect(Engine::mixer(), SIGNAL(sampleRateChanged()), this,
			SLOT(reloadPlugin()));

		if(multiChannelLinkModel()) {
			connect(multiChannelLinkModel(), SIGNAL(dataChanged()),
				this, SLOT(updateLinkStatesFromGlobal()));
			connect(getGroup(0), SIGNAL(linkStateChanged(int, bool)),
					this, SLOT(linkPort(int, bool)));
		}
	}
}

void SpaFxControls::setNameFromFile(const QString &name)
{
	effect()->setDisplayName(name);
}

void SpaFxControls::changeControl() // TODO: what is that?
{
	//	engine::getSong()->setModified();
}

void SpaFxControls::saveSettings(QDomDocument &doc, QDomElement &that)
{
	SpaControlBase::saveSettings(doc, that);
}

void SpaFxControls::loadSettings(const QDomElement &that)
{
	SpaControlBase::loadSettings(that);
}

int SpaFxControls::controlCount()
{
	std::size_t res = 0;
	for (const std::unique_ptr<SpaProc>& c : m_procs) { res += c->controlCount(); }
	return static_cast<int>(res);
}

EffectControlDialog *SpaFxControls::createView()
{
	return new SpaFxControlDialog(this);
}
