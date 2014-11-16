/*
 * dynamics_processor_controls.h - controls for dynamics_processor-effect
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef DYNPROC_CONTROLS_H
#define DYNPROC_CONTROLS_H

#include "EffectControls.h"
#include "dynamics_processor_control_dialog.h"
#include "knob.h"
#include "graph.h"

class dynProcEffect;


class dynProcControls : public EffectControls
{
	Q_OBJECT
public:
	enum StereoModes
	{
		SM_Maximum,
		SM_Average,
		SM_Unlinked,
		NumStereoModes
	};
	dynProcControls( dynProcEffect * _eff );
	virtual ~dynProcControls()
	{
	}

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );
	inline virtual QString nodeName() const
	{
		return( "dynamicsprocessor_controls" );
	}

	virtual void setDefaultShape();

	virtual int controlCount()
	{
		return( 6 );
	}

	virtual EffectControlDialog * createView()
	{
		return( new dynProcControlDialog( this ) );
	}


private slots:
	void samplesChanged( int, int );
	void sampleRateChanged();

	void resetClicked();
	void smoothClicked();

	void addOneClicked();
	void subOneClicked();

private:
	dynProcEffect * m_effect;
	
	FloatModel m_inputModel;
	FloatModel m_outputModel;
	FloatModel m_attackModel;
	FloatModel m_releaseModel;
	graphModel m_wavegraphModel;
	IntModel m_stereomodeModel;

	friend class dynProcControlDialog;
	friend class dynProcEffect;

} ;

#endif
