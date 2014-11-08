/*
 * MidiTime.h - declaration of class MidiTime which provides data type for
 *              position- and length-variables
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _MIDI_TIME_H
#define _MIDI_TIME_H

#include "lmms_basics.h"
#include "export.h"

const int DefaultTicksPerTact = 192;
const int DefaultStepsPerTact = 16;
const int DefaultBeatsPerTact = DefaultTicksPerTact / DefaultStepsPerTact;


class EXPORT MidiTime
{
public:
	MidiTime( const tact_t tact, const tick_t ticks ) :
		m_ticks( tact * s_ticksPerTact + ticks )
	{
	}

	MidiTime( const tick_t ticks = 0 ) :
		m_ticks( ticks )
	{
	}

	MidiTime( const MidiTime& time ) :
		m_ticks( time.m_ticks )
	{
	}

	MidiTime toNearestTact() const
	{
		if( m_ticks % s_ticksPerTact >= s_ticksPerTact/2 )
		{
			return ( getTact() + 1 ) * s_ticksPerTact;
		}
		return getTact() * s_ticksPerTact;
	}

	MidiTime toAbsoluteTact() const
	{
		return getTact() * s_ticksPerTact;
	}

	MidiTime& operator=( const MidiTime& time )
	{
		m_ticks = time.m_ticks;
		return *this;
	}

	MidiTime& operator+=( const MidiTime& time )
	{
		m_ticks += time.m_ticks;
		return *this;
	}

	MidiTime& operator-=( const MidiTime& time )
	{
		m_ticks -= time.m_ticks;
		return *this;
	}

	tact_t getTact() const
	{
		return m_ticks / s_ticksPerTact;
	}

	tact_t nextFullTact() const
	{
		if( m_ticks % s_ticksPerTact == 0 )
		{
			return m_ticks / s_ticksPerTact;
		}
		return m_ticks / s_ticksPerTact + 1;
	}

	void setTicks( tick_t ticks )
	{
		m_ticks = ticks;
	}

	tick_t getTicks() const
	{
		return m_ticks;
	}

	operator int() const
	{
		return m_ticks;
	}

	// calculate number of frame that are needed this time
	f_cnt_t frames( const float framesPerTick ) const
	{
		if( m_ticks >= 0 )
		{
			return static_cast<f_cnt_t>( m_ticks * framesPerTick );
		}
		return 0;
	}

	static MidiTime fromFrames( const f_cnt_t frames, const float framesPerTick )
	{
		return MidiTime( static_cast<int>( frames / framesPerTick ) );
	}


	static tick_t ticksPerTact()
	{
		return s_ticksPerTact;
	}

	static int stepsPerTact()
	{
		return qMax( 1, ticksPerTact() / DefaultBeatsPerTact );
	}

	static void setTicksPerTact( tick_t _tpt )
	{
		s_ticksPerTact = _tpt;
	}


private:
	tick_t m_ticks;

	static tick_t s_ticksPerTact;

} ;


#endif

