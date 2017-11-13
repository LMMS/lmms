/*
 * MultitapEchoControls.h - a multitap echo delay plugin
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
 
#ifndef MULTITAP_ECHO_CONTROLS_H
#define MULTITAP_ECHO_CONTROLS_H

#include "EffectControls.h"
#include "MultitapEchoControlDialog.h"
#include "Knob.h"
#include "Graph.h"


class MultitapEchoEffect;

class MultitapEchoControls : public EffectControls
{
	Q_OBJECT
public:
	MultitapEchoControls( MultitapEchoEffect * eff );
	virtual ~MultitapEchoControls();

	virtual void saveSettings( QDomDocument & doc, QDomElement & parent );
	virtual void loadSettings( const QDomElement & elem );
	inline virtual QString nodeName() const
	{
		return( "multitapechocontrols" );
	}

	void setDefaultAmpShape();
	void setDefaultLpShape();

	virtual int controlCount()
	{
		return( 5 );
	}

	virtual EffectControlDialog * createView()
	{
		return( new MultitapEchoControlDialog( this ) );
	}

private slots:
	void ampSamplesChanged( int, int );
	void ampResetClicked();
	
	void lpSamplesChanged( int, int );
	void lpResetClicked();
	
	void lengthChanged();
	void sampleRateChanged();

private:
	MultitapEchoEffect * m_effect;
	IntModel m_steps;
	TempoSyncKnobModel m_stepLength;
	
	FloatModel m_dryGain;
	BoolModel m_swapInputs;
	FloatModel m_stages;
	
	graphModel m_ampGraph;
	graphModel m_lpGraph;

	friend class MultitapEchoEffect;
	friend class MultitapEchoControlDialog;
};


#endif
