/*
 * StereoMatrixControls.h - controls for StereoMatrix effect
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail/dot/com>
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

#ifndef _STEREO_MATRIX_CONTROLS_H
#define _STEREO_MATRIX_CONTROLS_H

#include "EffectControls.h"
#include "StereoMatrixControlDialog.h"

namespace lmms
{


class StereoMatrixEffect;

class StereoMatrixControls : public EffectControls
{
	Q_OBJECT
public:
	StereoMatrixControls( StereoMatrixEffect( * _eff ) ); 
	~StereoMatrixControls() override = default;

	void saveSettings( QDomDocument & _doc, QDomElement & _parent ) override;
	void loadSettings( const QDomElement & _this ) override;
	inline QString nodeName() const override
	{
		return( "stereomatrixcontrols" );
	}

	int controlCount() override
	{
		return( 1 );
	}
	
	gui::EffectControlDialog* createView() override
	{
		return new gui::StereoMatrixControlDialog( this );
	}


private slots:
	void changeMatrix();


private:
	StereoMatrixEffect * m_effect;

	FloatModel m_llModel;
	FloatModel m_lrModel;
	FloatModel m_rlModel;
	FloatModel m_rrModel;
	
	friend class gui::StereoMatrixControlDialog;
	friend class StereoMatrixEffect;

} ;


} // namespace lmms

#endif 
