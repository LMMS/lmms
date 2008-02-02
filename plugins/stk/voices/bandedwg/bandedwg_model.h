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
#ifndef _BANDEDWG_MODEL_H
#define _BANDEDWG_MODEL_H

#include "stk_model.h"


class bandedWGModel: public stkModel
{
public:
	bandedWGModel( bool _monophonic = FALSE,
				StkFloat _portamento = 0.0f,
				StkFloat _bend = 0.0f,
				StkFloat _bend_range = 2.0f,
				bool _velocity_sensitive_lpf = TRUE,
				StkFloat _velocity_sensitive_q = 0.5f,
				StkFloat _volume = 1.0f,
				StkFloat _pan = 0.0f,
				StkFloat _spread = 0.0f,
				StkFloat _bow_pressure = 64.0f,
				StkFloat _bow_position = 64.0f,
				StkFloat _vibrato_frequency = 64.0f,
				StkFloat _vibrato_gain = 64.0f,
				StkFloat _bow_velocity = 64.0f,
				StkFloat _set_strike = 64.0f,
				StkFloat _sound = 0.0f );
	~bandedWGModel();
	
	inline floatModel * bowPressure( void ) const
	{
		return( m_bowPressure );
	}
		
	inline floatModel * bowPosition( void ) const
	{
		return( m_bowPosition );
	}
	
	inline floatModel * vibratoFrequency( void ) const
	{
		return( m_vibratoFrequency );
	}
	
	inline floatModel * vibratoGain( void ) const
	{
		return( m_vibratoGain );
	}
	
	inline floatModel * bowVelocity( void ) const
	{
		return( m_bowVelocity );
	}
	
	inline floatModel * setStrike( void ) const
	{
		return( m_setStrike );
	}
	
	inline floatModel * sound( void ) const
	{
		return( m_sound );
	}
	
	void FASTCALL saveSettings( QDomDocument & _doc, QDomElement & _parent );
	
	void FASTCALL loadSettings( const QDomElement & _this );

private:
	floatModel * m_bowPressure;
	floatModel * m_bowPosition;
	floatModel * m_vibratoFrequency;
	floatModel * m_vibratoGain;
	floatModel * m_bowVelocity;
	floatModel * m_setStrike;
	floatModel * m_sound;
};

#endif
