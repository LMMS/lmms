/*
 * ExampleEffectControls.h - Example effect control boilerplate code
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_EXAMPLE_EFFECT_CONTROLS_H
#define LMMS_EXAMPLE_EFFECT_CONTROLS_H

#include "EffectControls.h"
#include "ExampleEffectControlDialog.h"

namespace lmms
{

class ExampleEffectEffect;

namespace gui
{
class ExampleEffectControlDialog;
}

class ExampleEffectControls : public EffectControls
{
	Q_OBJECT
public:
	ExampleEffectControls(ExampleEffectEffect* effect);
	~ExampleEffectControls() override = default;

	void saveSettings(QDomDocument& doc, QDomElement& parent) override;
	void loadSettings(const QDomElement& parent) override;
	inline QString nodeName() const override
	{
		return "ExampleEffectControls";
	}
	gui::EffectControlDialog* createView() override
	{
		return new gui::ExampleEffectControlDialog(this);
	}
	int controlCount() override { return 5; }

private:
	ExampleEffectEffect* m_effect;
	
	//! Example volume knob model
	FloatModel m_volumeModel;
	//! Example normal knob model
	FloatModel m_decayModel;
	//! Example toggleable on/off model
	BoolModel m_invertModel;
	//! Example integer model
	IntModel m_numberModel;
	//! Example model for fader
	FloatModel m_faderModel;

	friend class gui::ExampleEffectControlDialog;
	friend class ExampleEffectEffect;
};

} // namespace lmms

#endif // LMMS_EXAMPLE_EFFECT_CONTROLS_H
