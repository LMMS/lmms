/*
 * ArpeggiatorControls.h - A native arpeggiatorer
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef ARPEGGIATOR_CONTROLS_H
#define ARPEGGIATOR_CONTROLS_H

#include "EffectControls.h"
#include "ArpeggiatorControlDialog.h"

class ArpeggiatorEffect;

class ArpeggiatorControls : public EffectControls
{
	Q_OBJECT
public:
	ArpeggiatorControls( ArpeggiatorEffect * eff );
	virtual ~ArpeggiatorControls();

	virtual void saveSettings( QDomDocument & doc, QDomElement & elem );
	virtual void loadSettings( const QDomElement & elem );
	inline virtual QString nodeName() const
	{
		return( "arpeggiatorcontrols" );
	}

	virtual int controlCount()
	{
		return( 9 );
	}

	virtual EffectControlDialog * createView()
	{
		return( new ArpeggiatorControlDialog( this ) );
	}

private slots:
	void sampleRateChanged();

private:
	ArpeggiatorEffect * m_effect;
	
	FloatModel m_inGain;
	FloatModel m_inNoise;
	
	FloatModel m_outGain;
	FloatModel m_outClip;
	
	FloatModel m_rate;
	FloatModel m_stereoDiff;
	
	FloatModel m_levels;
	
	BoolModel m_rateEnabled;
	BoolModel m_depthEnabled;
	
	friend class ArpeggiatorControlDialog;
	friend class ArpeggiatorEffect;
};

#endif
