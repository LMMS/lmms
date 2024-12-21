/*
 * DynamicsProcessor.h - DynamicsProcessor effect-plugin
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2006-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef DYNPROC_H
#define DYNPROC_H

#include "AudioPluginInterface.h"
#include "DynamicsProcessorControls.h"

namespace lmms
{

class RmsHelper;


class DynProcEffect : public DefaultEffectPluginInterface
{
public:
	DynProcEffect( Model * _parent,
			const Descriptor::SubPluginFeatures::Key * _key );
	~DynProcEffect() override;

	ProcessStatus processImpl(CoreAudioDataMut inOut) override;
	void processBypassedImpl() override;

	EffectControls * controls() override
	{
		return( &m_dpControls );
	}


private:
	void calcAttack();
	void calcRelease();

	DynProcControls m_dpControls;

// this member array is needed for peak detection 
	float m_currentPeak[2];
	double m_attCoeff;
	double m_relCoeff;
	
	bool m_needsUpdate;
	
	RmsHelper * m_rms [2];

	friend class DynProcControls;

} ;


} // namespace lmms

#endif
