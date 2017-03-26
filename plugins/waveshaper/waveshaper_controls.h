/*
 * waveshaper_controls.h - controls for waveshaper-effect
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef WAVESHAPER_CONTROLS_H
#define WAVESHAPER_CONTROLS_H

#include "EffectControls.h"
#include "waveshaper_control_dialog.h"
#include "Knob.h"
#include "Graph.h"

class waveShaperEffect;


class waveShaperControls : public EffectControls
{
	Q_OBJECT
public:
	waveShaperControls( waveShaperEffect * _eff );
	virtual ~waveShaperControls()
	{
	}

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );
	inline virtual QString nodeName() const
	{
		return( "waveshapercontrols" );
	}

	virtual void setDefaultShape();

	virtual int controlCount()
	{
		return( 4 );
	}

	virtual EffectControlDialog * createView()
	{
		return( new waveShaperControlDialog( this ) );
	}


private slots:
	void samplesChanged( int, int );

	void resetClicked();
	void smoothClicked();

	void addOneClicked();
	void subOneClicked();

private:
	waveShaperEffect * m_effect;
	FloatModel m_inputModel;
	FloatModel m_outputModel;
	graphModel m_wavegraphModel;
	BoolModel  m_clipModel;

	friend class waveShaperControlDialog;
	friend class waveShaperEffect;

} ;

#endif
