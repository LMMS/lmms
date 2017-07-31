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


private:
	ClickGDXControls m_gdxControls;
	uint32_t         m_current;

	friend class ClickGDXControls;

} ;

#endif
