/*
 *
 * Copyright (c) 2008 Danny McRae <khjklujn/at/users.sourceforge.net>
 * 
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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
#ifndef _RHODEY_MODEL_H
#define _RHODEY_MODEL_H

#include "stk_model.h"


class rhodeyModel: public stkModel
{
public:
	rhodeyModel( bool _monophonic = FALSE,
				StkFloat _portamento = 0.0f,
				StkFloat _bend = 0.0f,
				StkFloat _bend_range = 2.0f,
				bool _velocity_sensitive_lpf = TRUE,
				StkFloat _velocity_sensitive_q = 0.5f,
				StkFloat _volume = 1.0f,
				StkFloat _pan = 0.0f,
				StkFloat _spread = 0.0f,
				StkFloat _index = 64.0f,
				StkFloat _crossfade = 64.0f,
				StkFloat _lfo_speed = 64.0f,
				StkFloat _lfo_depth = 64.0f,
				StkFloat _adsr_target = 64.0f );
	~rhodeyModel();
	
	inline floatModel * index( void ) const
	{ 
		return( m_index ); 
	}
	
	inline floatModel * crossfade( void ) const
	{ 
		return( m_crossfade ); 
	}
	
	inline floatModel * lfoSpeed( void )
	{
		 return( m_lfoSpeed );
	}
	
	inline floatModel * lfoDepth( void ) const
	{
		 return( m_lfoDepth );
	}
	
	inline floatModel * adsrTarget( void ) const
	{
		return( m_adsrTarget );
	}
	
	void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	
	void loadSettings( const QDomElement & _this );

private:
	floatModel * m_index;
	floatModel * m_crossfade;
	floatModel * m_lfoSpeed;
	floatModel * m_lfoDepth;
	floatModel * m_adsrTarget;
};

#endif
