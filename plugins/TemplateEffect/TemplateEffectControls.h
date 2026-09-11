/*
 * TemplateEffectControls.h - Example effect control boilerplate code
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

#ifndef LMMS_TEMPLATE_EFFECT_CONTROLS_H
#define LMMS_TEMPLATE_EFFECT_CONTROLS_H

#include "EffectControls.h"
#include "TemplateEffectControlDialog.h"

namespace lmms
{

class TemplateEffectEffect;

namespace gui
{
class TemplateEffectControlDialog;
}

class TemplateEffectControls : public EffectControls
{
	Q_OBJECT
public:
	TemplateEffectControls(TemplateEffectEffect* effect);
	~TemplateEffectControls() override = default;

	void saveSettings(QDomDocument& doc, QDomElement& parent) override;
	void loadSettings(const QDomElement& parent) override;
	inline QString nodeName() const override
	{
		return "TemplateEffectControls";
	}
	gui::EffectControlDialog* createView() override
	{
		return new gui::TemplateEffectControlDialog(this);
	}
	int controlCount() override { return TEMPLATE_NUM_KNOBS; }

private:
	TEMPLATE_KNOB_LOOP_START
	FloatModel m_modelTEMPLATE_KNOB_NUMBER;
	TEMPLATE_KNOB_LOOP_END

	TemplateEffectEffect* m_effect;

	friend class gui::TemplateEffectControlDialog;
	friend class TemplateEffectEffect;
};

} // namespace lmms

#endif // LMMS_TEMPLATE_EFFECT_CONTROLS_H
