/*
 * peak_controller_Effect.h - PeakController effect plugin
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail/dot/com>
 * 
 * This file is part of LMMS - http://lmms.io
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


#ifndef _PEAK_CONTROLLER_EFFECT_H
#define _PEAK_CONTROLLER_EFFECT_H

#include "Effect.h"
#include "peak_controller_effect_controls.h"

class PeakControllerEffect : public Effect
{
public:
	PeakControllerEffect( Model * parent, 
						const Descriptor::SubPluginFeatures::Key * _key );
	virtual ~PeakControllerEffect();
	virtual bool processAudioBuffer( sampleFrame * _buf,
									const fpp_t _frames );

	virtual EffectControls * controls()
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

	int m_effectId;

private:
	PeakControllerEffectControls m_peakControls;

	float m_lastSample;
	float m_lastRMS;
	bool m_lastRMSavail;

	PeakController * m_autoController;

	friend class PeakControllerEffectControls;

} ;

#endif
