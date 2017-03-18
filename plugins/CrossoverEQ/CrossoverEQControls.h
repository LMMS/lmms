/*
 * CrossoverEQControls.h - A native 4-band Crossover Equalizer 
 * good for simulating tonestacks or simple peakless (flat-band) equalization
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

#ifndef CROSSOVEREQ_CONTROLS_H
#define CROSSOVEREQ_CONTROLS_H

#include "EffectControls.h"
#include "CrossoverEQControlDialog.h"

class CrossoverEQEffect;

class CrossoverEQControls : public EffectControls
{
	Q_OBJECT
public:
	CrossoverEQControls( CrossoverEQEffect * eff );
	virtual ~CrossoverEQControls() {}

	virtual void saveSettings( QDomDocument & doc, QDomElement & elem );
	virtual void loadSettings( const QDomElement & elem );
	inline virtual QString nodeName() const
	{
		return( "crossoevereqcontrols" );
	}

	virtual int controlCount()
	{
		return( 11 );
	}

	virtual EffectControlDialog * createView()
	{
		return( new CrossoverEQControlDialog( this ) );
	}

private slots:
	void xover12Changed();
	void xover23Changed();
	void xover34Changed();
	void sampleRateChanged();

private:
	CrossoverEQEffect * m_effect;
	
	FloatModel m_xover12;
	FloatModel m_xover23;
	FloatModel m_xover34;
	
	FloatModel m_gain1;
	FloatModel m_gain2;
	FloatModel m_gain3;
	FloatModel m_gain4;
	
	BoolModel m_mute1;
	BoolModel m_mute2;
	BoolModel m_mute3;
	BoolModel m_mute4;
	
	friend class CrossoverEQControlDialog;
	friend class CrossoverEQEffect;
};

#endif
