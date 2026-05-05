/*
 * TimePos.h - declaration of class TimePos which provides data type for
 *             position- and length-variables
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

#ifndef LMMS_TIME_POS_H
#define LMMS_TIME_POS_H

#include <algorithm>
#include <cassert>
#include "lmms_export.h"
#include "LmmsTypes.h"

namespace lmms
{

// note: a bar was erroneously called "tact" in older versions of LMMS
const int DefaultTicksPerBar = 192;
const int DefaultStepsPerBar = 16;
const int DefaultBeatsPerBar = DefaultTicksPerBar / DefaultStepsPerBar;


class MeterModel;

/**
	Represents a time signature, in which the numerator is the number of beats
	in a bar, while the denominator is the type of note representing a beat.

	Example: 6/8 means 6 beats in a bar with each beat having a duration of one 8th-note.
*/
class LMMS_EXPORT TimeSig
{
public:
	TimeSig( int num, int denom );
	TimeSig( const MeterModel &model );
	int numerator() const { return m_num; }
	int denominator() const { return m_denom; }
private:
	int m_num;
	int m_denom;
};


/**
	Represents a position in time or length of a note or event, in ticks, beats, and bars
*/
class LMMS_EXPORT TimePos
{
public:
	TimePos( const bar_t bar, const tick_t ticks );
	TimePos( const tick_t ticks = 0 );

	TimePos quantize(float bars, bool forceRoundDown = false) const;
	TimePos toAbsoluteBar() const { return getBar() * s_ticksPerBar; }

	TimePos& operator+=(const TimePos& time)
	{
		m_ticks += time.m_ticks;
		return *this;
	}

	TimePos& operator-=(const TimePos& time)
	{
		m_ticks -= time.m_ticks;
		return *this;
	}

	// return the bar, rounded down and 0-based
	bar_t getBar() const { return m_ticks / s_ticksPerBar; }

	// return the bar, rounded up and 0-based
	bar_t nextFullBar() const { return (m_ticks + (s_ticksPerBar - 1)) / s_ticksPerBar; }

	void setTicks(tick_t ticks) { m_ticks = ticks; }
	tick_t getTicks() const { return m_ticks; }

	operator int() const { return m_ticks; }

	tick_t ticksPerBeat(const TimeSig& sig) const { return ticksPerBar(sig) / sig.numerator(); }

	// Remainder ticks after bar is removed
	tick_t getTickWithinBar(const TimeSig& sig) const { return m_ticks % ticksPerBar(sig); }

	// Returns the beat position inside the bar, 0-based
	tick_t getBeatWithinBar(const TimeSig& sig) const { return getTickWithinBar(sig) / ticksPerBeat(sig); }

	// Remainder ticks after bar and beat are removed
	tick_t getTickWithinBeat(const TimeSig& sig) const { return getTickWithinBar(sig) % ticksPerBeat(sig); }

	// calculate number of frame that are needed this time
	f_cnt_t frames(const float framesPerTick) const
	{
		// Before, step notes used to have negative length. This
		// assert is a safeguard against negative length being
		// introduced again (now using Note Types instead #5902)
		assert(m_ticks >= 0);
		return static_cast<f_cnt_t>(m_ticks * framesPerTick);
	}

	double getTimeInMilliseconds(bpm_t beatsPerMinute) const { return ticksToMilliseconds(getTicks(), beatsPerMinute); }

	static TimePos fromFrames(const f_cnt_t frames, const float framesPerTick)
	{
		return TimePos(static_cast<int>(frames / framesPerTick));
	}

	static tick_t ticksPerBar() { return s_ticksPerBar; }
	static tick_t ticksPerBar(const TimeSig& sig) { return DefaultTicksPerBar * sig.numerator() / sig.denominator(); }

	static int stepsPerBar() { return std::max(1, ticksPerBar() / DefaultBeatsPerBar); }
	static void setTicksPerBar(tick_t ticks) { s_ticksPerBar = ticks; }
	static TimePos stepPosition(int step) { return step * ticksPerBar() / stepsPerBar(); }

	static double ticksToMilliseconds(tick_t ticks, bpm_t beatsPerMinute)
	{
		return ticksToMilliseconds(static_cast<double>(ticks), beatsPerMinute);
	}

	static double ticksToMilliseconds(double ticks, bpm_t beatsPerMinute) { return (ticks * 1250) / beatsPerMinute; }

private:
	tick_t m_ticks;

	static tick_t s_ticksPerBar;

} ;


} // namespace lmms

#endif // LMMS_TIME_POS_H
