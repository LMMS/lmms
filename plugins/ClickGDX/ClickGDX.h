/*
 * ClickGDX.h - click remover effect
 *
 * Copyright (c) 2017
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


#ifndef CLICKGDX_H
#define CLICKGDX_H

#include "lmms_math.h"
#include "Effect.h"
#include "ClickGDXControls.h"
#include "ValueBuffer.h"

class ClickGDXEffect : public Effect
{
 public:
	ClickGDXEffect( Model* parent, const Descriptor::SubPluginFeatures::Key* key );
	virtual ~ClickGDXEffect();
	virtual bool processAudioBuffer( sampleFrame* buf, const fpp_t frames );

	virtual EffectControls* controls()
	{
		return &m_gdxControls;
	}

 protected:
	/*
	  t must be between 0.f and 1.f.
	  type:
	  0 square 0
	  1 square 1
	  2 linear x
	  3 sin(pi/2*x)
	  4 (1.19+atan(2*x-2.5))*1.3755
	  5 (1.+sin(pi*(x-0.5)))*0.5.
	  6 (1.19+atan(10*x-2.5))*0.38023
	*/
	static inline float f0(float t) { return 0.f; }
	static inline float f1(float t) { return 1.f; }
	static inline float f2(float t) { return t; }
	static inline float f3(float t) { return sin(F_PI_2*t); }
	static inline float f4(float t) { return (1.19f+atan(2.f*t-2.5f))*1.3755f; }
	static inline float f5(float t) { return (1.5f+sin(F_PI*(t-0.5f)))*0.5f; }
	static inline float f6(float t) { return (1.19f+atan(10.f*t-2.5f))*0.38023f; }

 private:
	ClickGDXControls m_gdxControls;
	uint32_t         m_volCurrent;
	uint32_t         m_panCurrent;

	friend class ClickGDXControls;

} ;

#endif
