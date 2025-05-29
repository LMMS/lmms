/*
 * TemplateEffect.h - Example effect boilerplate code
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_TEMPLATE_EFFECT_H
#define LMMS_TEMPLATE_EFFECT_H

#include "Effect.h"
#include "TemplateEffectControls.h"

namespace lmms
{

class TemplateEffectEffect : public Effect
{
public:
	TemplateEffectEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key);
	~TemplateEffectEffect() override = default;

	ProcessStatus processImpl(SampleFrame* buf, const fpp_t frames) override;

	EffectControls* controls() override
	{
		return &m_controls;
	}

private:
	TemplateEffectControls m_controls;

	SampleFrame m_smoothedValue;

	friend class TemplateEffectControls;
};

} // namespace lmms

#endif // LMMS_TEMPLATE_EFFECT_H
