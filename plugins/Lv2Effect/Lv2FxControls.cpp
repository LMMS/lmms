/*
 * Lv2Controls.cpp - controls for amplifier effect
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

#include "Lv2FxControls.h"

#include <QDomElement>

#include "Engine.h"
#include "Lv2Effect.h"
#include "Lv2FxControlDialog.h"
#include "Lv2Proc.h"




Lv2FxControls::Lv2FxControls(class Lv2Effect *effect, const QString& uri) :
	EffectControls(effect),
	Lv2ControlBase(this, uri),
	m_effect(effect)
{
	if (isValid())
	{
		connect(Engine::mixer(), &Mixer::sampleRateChanged,
			this, [this](){Lv2ControlBase::reloadPlugin();});
		if (multiChannelLinkModel())
		{
			connect(multiChannelLinkModel(), &BoolModel::dataChanged,
				this, [this](){updateLinkStatesFromGlobal();},
				Qt::DirectConnection);
/*			// linking models will be deprecated soon, anyways (#5079 voting)
			connect(getGroup(0), &LinkedModelGroup::linkStateChanged,
				this, [this](std::size_t id, bool value){
				linkModel(id, value);}, Qt::DirectConnection);*/
		}
	}
}




void Lv2FxControls::saveSettings(QDomDocument &doc, QDomElement &that)
{
	Lv2ControlBase::saveSettings(doc, that);
}




void Lv2FxControls::loadSettings(const QDomElement &that)
{
	Lv2ControlBase::loadSettings(that);
}




int Lv2FxControls::controlCount()
{
	return static_cast<int>(Lv2ControlBase::controlCount());
}




EffectControlDialog *Lv2FxControls::createView()
{
	return new Lv2FxControlDialog(this);
}




void Lv2FxControls::changeControl() // TODO: what is that?
{
	//	engine::getSong()->setModified();
}




DataFile::Types Lv2FxControls::settingsType()
{
	return DataFile::EffectSettings;
}




void Lv2FxControls::setNameFromFile(const QString &name)
{
	effect()->setDisplayName(name);
}


