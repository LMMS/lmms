/*
 * stereoenhancer_controls.h - controls for stereoEnhancer-effect
 *
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

#ifndef _STEREO_ENHANCER_CONTROLS_H
#define _STEREO_ENHANCER_CONTROLS_H

#include "EffectControls.h"
#include "stereoenhancer_control_dialog.h"
#include "knob.h"

class stereoEnhancerEffect;

class stereoEnhancerControls : public EffectControls
{
	Q_OBJECT
public:
	stereoEnhancerControls( stereoEnhancerEffect( * _eff ) ); 
	virtual ~stereoEnhancerControls()
	{
	}

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );
	inline virtual QString nodeName() const
	{
		return( "stereoenhancercontrols" );
	}

	virtual int controlCount()
	{
		return( 1 );
	}
	
	virtual EffectControlDialog * createView()
	{
		return new stereoEnhancerControlDialog( this );
	}


private slots:
	void changeWideCoeff();


private:
	stereoEnhancerEffect * m_effect;
	FloatModel m_widthModel;
	
	friend class stereoEnhancerControlDialog;

} ;


#endif /*_STEREO_ENHANCER_CONTROLS_H*/
