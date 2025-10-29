/*
 * StereoEnhancerControls.h - controls for StereoEnhancer effect
 *
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

#ifndef _STEREO_ENHANCER_CONTROLS_H
#define _STEREO_ENHANCER_CONTROLS_H

#include "EffectControls.h"
#include "StereoEnhancerControlDialog.h"

namespace lmms
{


class StereoEnhancerEffect;

class StereoEnhancerControls : public EffectControls
{
	Q_OBJECT
public:
	StereoEnhancerControls( StereoEnhancerEffect( * _eff ) ); 
	~StereoEnhancerControls() override = default;

	void saveSettings( QDomDocument & _doc, QDomElement & _parent ) override;
	void loadSettings( const QDomElement & _this ) override;
	inline QString nodeName() const override
	{
		return( "stereoenhancercontrols" );
	}

	int controlCount() override
	{
		return( 1 );
	}
	
	gui::EffectControlDialog* createView() override
	{
		return new gui::StereoEnhancerControlDialog( this );
	}


private slots:
	void changeWideCoeff();


private:
	StereoEnhancerEffect * m_effect;
	FloatModel m_widthModel;
	
	friend class gui::StereoEnhancerControlDialog;

} ;


} // namespace lmms

#endif /*_STEREO_ENHANCER_CONTROLS_H*/
