/*
 * SpaEffect.h - implementation of SPA effect
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

#ifndef SPA_EFFECT_H
#define SPA_EFFECT_H

#include <QString>

#include "Effect.h"
#include "SpaFxControls.h"

class SpaEffect : public Effect
{
	Q_OBJECT

public:
	SpaEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* _key);
	~SpaEffect() override;

	bool processAudioBuffer( sampleFrame* buf, const fpp_t frames ) override;

	EffectControls* controls() override { return &m_controls; }

	SpaFxControls* spaControls() { return &m_controls; }
	const SpaFxControls* spaControls() const { return &m_controls; }

	unsigned netPort(std::size_t chan) const override;
	class AutomatableModel* modelAtPort(const QString& dest) override;

private:	
	SpaFxControls m_controls;

	friend class SpaFxControls;
};

#endif // LMMS_HAVE_SPA
