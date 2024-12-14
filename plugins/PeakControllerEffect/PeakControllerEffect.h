/*
 * peak_controller_Effect.h - PeakController effect plugin
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail/dot/com>
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


#ifndef PEAK_CONTROLLER_EFFECT_H
#define PEAK_CONTROLLER_EFFECT_H

#include "AudioPluginInterface.h"
#include "PeakControllerEffectControls.h"

namespace lmms
{


class PeakController;

class PeakControllerEffect : public DefaultEffectPluginInterface
{
public:
	PeakControllerEffect( Model * parent, 
						const Descriptor::SubPluginFeatures::Key * _key );
	~PeakControllerEffect() override;

	ProcessStatus processImpl(CoreAudioDataMut inOut) override;

	EffectControls * controls() override
	{
		return &m_peakControls;
	}

	float lastSample()
	{
		return m_lastSample;
	}

	PeakController * controller()
	{
		return m_autoController;
	}
	
	FloatModel * attackModel()
	{
		return &( m_peakControls.m_attackModel );
	}

	FloatModel * decayModel()
	{
		return &( m_peakControls.m_decayModel );
	}

	int m_effectId;

private:
	PeakControllerEffectControls m_peakControls;

	float m_lastSample;

	PeakController * m_autoController;

	friend class PeakControllerEffectControls;

} ;


} // namespace lmms

#endif
