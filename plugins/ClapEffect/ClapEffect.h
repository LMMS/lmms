/*
 * ClapEffect.h - implementation of CLAP effect
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

#ifndef LMMS_CLAP_EFFECT_H
#define LMMS_CLAP_EFFECT_H

#include "Effect.h"
//#include "ClapFxControls.h"

namespace lmms
{


class ClapEffect : public Effect
{
	Q_OBJECT

public:
	ClapEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* _key);

	//! Must be checked after ctor or reload
	auto isValid() const -> bool { return true;/*m_controls.isValid();*/ } // TODO

	auto processAudioBuffer(sampleFrame* buf, const fpp_t frames) -> bool override;
	auto controls() -> EffectControls* override { return nullptr; /*&m_controls;*/ } // TODO

	//auto clapControls() -> ClapFxControls* { return &m_controls; } // TODO
	//auto clapControls() const -> const ClapFxControls* { return &m_controls; } // TODO

private:
	//ClapFxControls m_controls; // TODO
	std::vector<sampleFrame> m_tempOutputSamples;
};


} // namespace lmms

#endif // LMMS_CLAP_EFFECT_H
