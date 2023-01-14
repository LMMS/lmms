/*
 * Lv2Effect.h - implementation of LV2 effect
 *
 * Copyright (c) 2018-2020 Johannes Lorenz <jlsf2013$users.sourceforge.net, $=@>
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

#ifndef LV2_EFFECT_H
#define LV2_EFFECT_H

#include "Effect.h"
#include "Lv2FxControls.h"

namespace lmms
{


class Lv2Effect : public Effect
{
	Q_OBJECT

public:
	/*
		initialization
	*/
	Lv2Effect(Model* parent, const Descriptor::SubPluginFeatures::Key* _key);
	//! Must be checked after ctor or reload
	bool isValid() const { return m_controls.isValid(); }

	bool processAudioBuffer( sampleFrame* buf, const fpp_t frames ) override;
	EffectControls* controls() override { return &m_controls; }

	Lv2FxControls* lv2Controls() { return &m_controls; }
	const Lv2FxControls* lv2Controls() const { return &m_controls; }

private:
	Lv2FxControls m_controls;
	std::vector<sampleFrame> m_tmpOutputSmps;
};


} // namespace lmms

#endif
