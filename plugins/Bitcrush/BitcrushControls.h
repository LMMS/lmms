/*
 * BitcrushControls.h - A native bitcrusher
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

#ifndef BITCRUSH_CONTROLS_H
#define BITCRUSH_CONTROLS_H

#include "EffectControls.h"
#include "BitcrushControlDialog.h"

namespace lmms
{


class BitcrushEffect;

class BitcrushControls : public EffectControls
{
	Q_OBJECT
public:
	BitcrushControls( BitcrushEffect * eff );
	~BitcrushControls() override = default;

	void saveSettings( QDomDocument & doc, QDomElement & elem ) override;
	void loadSettings( const QDomElement & elem ) override;
	inline QString nodeName() const override
	{
		return( "bitcrushcontrols" );
	}

	int controlCount() override
	{
		return( 9 );
	}

	gui::EffectControlDialog * createView() override
	{
		return( new gui::BitcrushControlDialog( this ) );
	}

private slots:
	void sampleRateChanged();

private:
	BitcrushEffect * m_effect;
	
	FloatModel m_inGain;
	FloatModel m_inNoise;
	
	FloatModel m_outGain;
	FloatModel m_outClip;
	
	FloatModel m_rate;
	FloatModel m_stereoDiff;
	
	FloatModel m_levels;
	
	BoolModel m_rateEnabled;
	BoolModel m_depthEnabled;
	
	friend class gui::BitcrushControlDialog;
	friend class BitcrushEffect;
};


} // namespace lmms

#endif
