/*
 * Lv2FxControls.cpp - Lv2FxControls implementation
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

#include "Lv2FxControls.h"


#include "Engine.h"
#include "Lv2Effect.h"
#include "Lv2FxControlDialog.h"

namespace lmms
{


Lv2FxControls::Lv2FxControls(class Lv2Effect *effect, const QString& uri) :
	EffectControls(effect),
	Lv2ControlBase(this, uri)
{
	connect(Engine::audioEngine(), &AudioEngine::sampleRateChanged,
		this, &Lv2FxControls::onSampleRateChanged);
}




void Lv2FxControls::reload()
{
	Lv2ControlBase::reload();
	emit modelChanged();
}




void Lv2FxControls::onSampleRateChanged()
{
	// TODO: once lv2 options are implemented,
	//       plugins that support it might allow changing their samplerate
	//       through it instead of reloading
	reload();
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




gui::EffectControlDialog *Lv2FxControls::createView()
{
	return new gui::Lv2FxControlDialog(this);
}




void Lv2FxControls::changeControl() // TODO: what is that?
{
	//	engine::getSong()->setModified();
}


} // namespace lmms
