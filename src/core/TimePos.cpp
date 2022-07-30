/*
 * TimePos.cpp - Class that encapsulates the position of a note/event in terms of
 *   its bar, beat and tick.
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net
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

#include "TimePos.h"

#include "MeterModel.h"

namespace lmms
{

TimeSig::TimeSig( int num, int denom ) :
	m_num(num),
	m_denom(denom)
{
}

TimeSig::TimeSig( const MeterModel &model ) :
	m_num(model.getNumerator()),
	m_denom(model.getDenominator())
{
}


int TimeSig::numerator() const
{
	return m_num;
}

int TimeSig::denominator() const
{
	return m_denom;
}




TimePos::TimePos( const bar_t bar, const tick_t ticks ) :
	m_ticks( bar * s_ticksPerBar + ticks )
{
}

TimePos::TimePos( const tick_t ticks ) :
	m_ticks( ticks )
{
}

TimePos TimePos::quantize(float bars) const
{
	//The intervals we should snap to, our new position should be a factor of this
	int interval = s_ticksPerBar * bars;
	//The lower position we could snap to
	int lowPos = m_ticks / interval;
	//Offset from the lower position
	int offset = m_ticks % interval;
	//1 if we should snap up, 0 if we shouldn't
	// Ternary expression is making sure that the snap happens in the direction to
	// the right even if m_ticks is negative and the offset is exactly half-way
	// More details on issue #5840 and PR #5847
	int snapUp = ((2 * offset) == -interval)
		? 0
		: (2 * offset) / interval;

	return (lowPos + snapUp) * interval;
}


TimePos TimePos::toAbsoluteBar() const
{
	return getBar() * s_ticksPerBar;
}


TimePos& TimePos::operator+=( const TimePos& time )
{
	m_ticks += time.m_ticks;
	return *this;
}


TimePos& TimePos::operator-=( const TimePos& time )
{
	m_ticks -= time.m_ticks;
	return *this;
}


bar_t TimePos::getBar() const
{
	return m_ticks / s_ticksPerBar;
}


bar_t TimePos::nextFullBar() const
{
	return ( m_ticks + ( s_ticksPerBar - 1 ) ) / s_ticksPerBar;
}


void TimePos::setTicks( tick_t ticks )
{
	m_ticks = ticks;
}


tick_t TimePos::getTicks() const
{
	return m_ticks;
}


TimePos::operator int() const
{
	return m_ticks;
}


tick_t TimePos::ticksPerBeat( const TimeSig &sig ) const
{
	// (number of ticks per bar) divided by (number of beats per bar)
	return ticksPerBar(sig) / sig.numerator();
}


tick_t TimePos::getTickWithinBar( const TimeSig &sig ) const
{
	return m_ticks % ticksPerBar( sig );
}

tick_t TimePos::getBeatWithinBar( const TimeSig &sig ) const
{
	return getTickWithinBar( sig ) / ticksPerBeat( sig );
}

tick_t TimePos::getTickWithinBeat( const TimeSig &sig ) const
{
	return getTickWithinBar( sig ) % ticksPerBeat( sig );
}


f_cnt_t TimePos::frames( const float framesPerTick ) const
{
	if( m_ticks >= 0 )
	{
		return static_cast<f_cnt_t>( m_ticks * framesPerTick );
	}
	return 0;
}

double TimePos::getTimeInMilliseconds( bpm_t beatsPerMinute ) const
{
	return ticksToMilliseconds( getTicks(), beatsPerMinute );
}

TimePos TimePos::fromFrames( const f_cnt_t frames, const float framesPerTick )
{
	return TimePos( static_cast<int>( frames / framesPerTick ) );
}


tick_t TimePos::ticksPerBar()
{
	return s_ticksPerBar;
}


tick_t TimePos::ticksPerBar( const TimeSig &sig )
{
	return DefaultTicksPerBar * sig.numerator() / sig.denominator();
}


int TimePos::stepsPerBar()
{
	int steps = ticksPerBar() / DefaultBeatsPerBar;
	return qMax( 1, steps );
}


void TimePos::setTicksPerBar( tick_t tpb )
{
	s_ticksPerBar = tpb;
}


TimePos TimePos::stepPosition( int step )
{
	return step * ticksPerBar() / stepsPerBar();
}

double TimePos::ticksToMilliseconds( tick_t ticks, bpm_t beatsPerMinute )
{
	return TimePos::ticksToMilliseconds( static_cast<double>(ticks), beatsPerMinute );
}

double TimePos::ticksToMilliseconds(double ticks, bpm_t beatsPerMinute)
{
	// 60 * 1000 / 48 = 1250
	return ( ticks * 1250 ) / beatsPerMinute;
}


} // namespace lmms