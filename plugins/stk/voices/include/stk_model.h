/*
 * stk_model.h - base class for stk instrument models
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
#ifndef _STK_MODEL_H
#define _STK_MODEL_H

#include "Instrmnt.h"

#include "automatable_model_templates.h"
#include "basic_filters.h"


class stkModel: public model
{
public:
	stkModel( bool _monophonic = FALSE,
			StkFloat _portamento = 0.0f, 
			StkFloat _bend = 0.0f, 
			StkFloat _bend_range = 2.0f, 
			bool _velocity_sensitive_lpf = TRUE,
			StkFloat _velocity_sensitive_q = 0.5f,
			StkFloat _volume = 1.0f,
			StkFloat _pan = 0.0f,
			bool _release_triggered = FALSE,
			bool _randomize_attack = FALSE,
			StkFloat _randomize_length = 0.0f,
			StkFloat _randomize_velocity_amount = 0.5f,
			StkFloat _randomize_frequency_amount = 0.5f,
			StkFloat _spread = 0.0f );
	virtual ~stkModel();
	
	// Indicates whether or not the instrument should use multiple voices
	// when generating the signal.
	inline boolModel * monophonic( void ) const
	{ 
		return( m_monophonic ); 
	}
	
	// Stores the time in ms over which a note will slide from the previous
	// frequency.
	inline floatModel * portamento( void ) const
	{ 
		return( m_portamento ); 
	}
	
	// Stores the pitch bend setting.  Scaled by bendRange.
	inline floatModel * bend( void ) const
	{ 
		return( m_bend ); 
	}
	
	// Stores the range in half steps over which a note can be bent.
	inline floatModel * bendRange( void ) const
	{ 
		return( m_bendRange ); 
	}
	
	// Indicates whether or not to apply a low pass filter to the notes 
	// where the cutoff frequency is scaled by the notes' velocities.
	inline boolModel * velocitySensitiveLPF( void ) const
	{ 
		return( m_velocitySensitiveLPF ); 
	}
	
	// Stores the Q value for the low pass filter.
	inline floatModel * velocitySensitiveQ( void ) const
	{ 
		return( m_velocitySensitiveQ ); 
	}
	
	// Stores the volume setting for the voice.
	inline floatModel * volume( void ) const
	{ 
		return( m_volume ); 
	}
	
	// Stores the pan setting for the voice.
	inline floatModel * pan( void ) const
	{ 
		return( m_pan ); 
	}
	
	// Indicates whether or not only the release portion of the envelope
	// should be played.
	inline boolModel * releaseTriggered( void ) const
	{ 
		return( m_releaseTriggered ); 
	}
	
	// Indicates whether or not random offsets should be generated for
	// the volume and pitch.  The offsets will move linearly to
	// zero over the length of time defined by randomizeLength.
	inline boolModel * randomizeAttack( void ) const
	{ 
		return( m_randomizeAttack ); 
	}
	
	// Stores the length of time in ms over which random offsets should
	// be applied.
	inline floatModel * randomizeLength( void ) const
	{ 
		return( m_randomizeLength );
	}
	
	// Stores the range for the random offset generated for the
	// volume of a note.
	inline floatModel * randomizeVelocityAmount( void ) const
	{
		 return( m_randomizeVelocityAmount );
	}
	
	// Stores the range in half steps for the random offset generated
	// for the pitch of a note.
	inline floatModel * randomizeFrequencyAmount( void ) const
	{
		 return( m_randomizeFrequencyAmount ); 
	}
	
	// Sets a delay for the right channel.  Provides a quick and dirty
	// way to create a stereo spread effect.
	inline floatModel * spread( void ) const
	{
		 return( m_spread );
	}
	
	// Calculates the amount to scale a frequency in order to provide
	// a "bend" in a note.
	inline float bendScaler() const
	{
		return( pow( 2.0f, ( m_bendRange->value() * m_bend->value() /
							49152.0f ) ) );
	}
	
	// Generates a random number between -1.0 and 1.0.
	inline float offset( void ) const
	{
		return( 2.0f * ( static_cast<float>( rand() ) / 
				static_cast<float>( RAND_MAX ) - 0.5f ) );
	}
	
	virtual void FASTCALL saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void FASTCALL loadSettings( const QDomElement & _this );
 
private:
	boolModel * m_monophonic;		// multiple voice indicator
	floatModel * m_portamento;		// time for slide between notes
	floatModel * m_bend; 			// bend state of note
	floatModel * m_bendRange;		// range of bend in half steps
	boolModel * m_velocitySensitiveLPF;	// turns on a LPF
	floatModel * m_velocitySensitiveQ;	// Q setting for the LPF
	floatModel * m_volume;			// volume of the voice
	floatModel * m_pan;			// pan position of the voice
	boolModel * m_releaseTriggered;		// only play release
	boolModel * m_randomizeAttack;		// turns on randomization
	floatModel * m_randomizeLength;		// time for randomization
	floatModel * m_randomizeVelocityAmount;	// velocity randomization
	floatModel * m_randomizeFrequencyAmount;// pitch randomization
	floatModel * m_spread;			// stereo width
};



#endif
