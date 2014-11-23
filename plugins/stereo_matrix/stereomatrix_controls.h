/*
 * stereomatrix_controls.h - controls for stereoMatrix-effect
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail/dot/com>
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

#ifndef _STEREO_MATRIX_CONTROLS_H
#define _STEREO_MATRIX_CONTROLS_H

#include "EffectControls.h"
#include "stereomatrix_control_dialog.h"
#include "knob.h"

class stereoMatrixEffect;

class stereoMatrixControls : public EffectControls
{
	Q_OBJECT
public:
	stereoMatrixControls( stereoMatrixEffect( * _eff ) ); 
	virtual ~stereoMatrixControls()
	{
	}

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );
	inline virtual QString nodeName() const
	{
		return( "stereomatrixcontrols" );
	}

	virtual int controlCount()
	{
		return( 1 );
	}
	
	virtual EffectControlDialog * createView()
	{
		return new stereoMatrixControlDialog( this );
	}


private slots:
	void changeMatrix();


private:
	stereoMatrixEffect * m_effect;

	FloatModel m_llModel;
	FloatModel m_lrModel;
	FloatModel m_rlModel;
	FloatModel m_rrModel;
	
	friend class stereoMatrixControlDialog;
	friend class stereoMatrixEffect;

} ;


#endif 
