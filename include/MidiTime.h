/*
 * MidiTime.h - declaration of class MidiTime which provides data type for
 *              position- and length-variables
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net
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


#ifndef MIDI_TIME_H
#define MIDI_TIME_H

#include <QtGlobal>

#include "export.h"
#include "lmms_basics.h"

// note: 1 "Tact" = 1 Measure
const int DefaultTicksPerTact = 192;
const int DefaultStepsPerTact = 16;
const int DefaultBeatsPerTact = DefaultTicksPerTact / DefaultStepsPerTact;


class MeterModel;

class EXPORT TimeSig
{
public:
	// in a time signature,
	// the numerator represents the number of beats in a measure.
	// the denominator indicates which type of note represents a beat.
	// example: 6/8 means 6 beats in a measure, where each beat has duration equal to one 8th-note.
	TimeSig( int num, int denom );
	TimeSig( const MeterModel &model );
	int numerator() const;
	int denominator() const;
private:
	int m_num;
	int m_denom;
};


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

	// return the tact, rounded down and 0-based
	tact_t getTact() const
	{
		return m_ticks / s_ticksPerTact;
	}

	tact_t nextFullTact() const
	{
		return (m_ticks + (s_ticksPerTact-1)) / s_ticksPerTact;
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

	tick_t ticksPerBeat( const TimeSig &sig ) const
	{
		return ticksPerTact(sig) / sig.numerator();
	}
	// Remainder ticks after bar is removed
	tick_t getTickWithinBar( const TimeSig &sig ) const
	{
		return m_ticks % ticksPerTact(sig);
	}
	// Returns the beat position inside the bar, 0-based
	tick_t getBeatWithinBar( const TimeSig &sig ) const
	{
		return getTickWithinBar(sig) / ticksPerBeat(sig);
	}
	// Remainder ticks after bar and beat are removed
	tick_t getTickWithinBeat( const TimeSig &sig ) const
	{
		return getTickWithinBar(sig) % ticksPerBeat(sig);
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
	static tick_t ticksPerTact( const TimeSig &sig ) const
	{
		return DefaultTicksPerTact * sig.numerator() / sig.denominator();
	}

	static int stepsPerTact()
	{
		int steps = ticksPerTact() / DefaultBeatsPerTact;
		return qMax( 1, steps );
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

