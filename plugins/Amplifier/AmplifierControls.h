/*
 * AmplifierControls.h - controls for bassboosterx -effect
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef AMPLIFIER_CONTROLS_H
#define AMPLIFIER_CONTROLS_H

#include "EffectControls.h"
#include "AmplifierControlDialog.h"
#include "Knob.h"


class AmplifierEffect;


class AmplifierControls : public EffectControls
{
	Q_OBJECT
public:
	AmplifierControls( AmplifierEffect* effect );
	virtual ~AmplifierControls()
	{
	}

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );
	inline virtual QString nodeName() const
	{
		return "AmplifierControls";
	}

	virtual int controlCount()
	{
		return 4;
	}

	virtual EffectControlDialog* createView()
	{
		return new AmplifierControlDialog( this );
	}


private slots:
	void changeControl();

private:
	AmplifierEffect* m_effect;
	FloatModel m_volumeModel;
	FloatModel m_panModel;
	FloatModel m_leftModel;
	FloatModel m_rightModel;

	friend class AmplifierControlDialog;
	friend class AmplifierEffect;

} ;

#endif
