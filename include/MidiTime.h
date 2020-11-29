/*
 * MidiTime.h - declaration of class MidiTime which provides data type for
 *              position- and length-variables
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


#ifndef MIDI_TIME_H
#define MIDI_TIME_H

#include <QtGlobal>

#include "lmms_export.h"
#include "lmms_basics.h"

// note: a bar was erroneously called "tact" in older versions of LMMS
const int DefaultTicksPerBar = 192;
const int DefaultStepsPerBar = 16;
const int DefaultBeatsPerBar = DefaultTicksPerBar / DefaultStepsPerBar;


class MeterModel;

class LMMS_EXPORT TimeSig
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


class LMMS_EXPORT MidiTime
{
public:
	MidiTime( const bar_t bar, const tick_t ticks );
	MidiTime( const tick_t ticks = 0 );

	MidiTime quantize(float) const;
	MidiTime toAbsoluteBar() const;

	MidiTime& operator+=( const MidiTime& time );
	MidiTime& operator-=( const MidiTime& time );

	// return the bar, rounded down and 0-based
	bar_t getBar() const;
	// return the bar, rounded up and 0-based
	bar_t nextFullBar() const;

	void setTicks( tick_t ticks );
	tick_t getTicks() const;

	operator int() const;

	tick_t ticksPerBeat( const TimeSig &sig ) const;
	// Remainder ticks after bar is removed
	tick_t getTickWithinBar( const TimeSig &sig ) const;
	// Returns the beat position inside the bar, 0-based
	tick_t getBeatWithinBar( const TimeSig &sig ) const;
	// Remainder ticks after bar and beat are removed
	tick_t getTickWithinBeat( const TimeSig &sig ) const;

	// calculate number of frame that are needed this time
	f_cnt_t frames( const float framesPerTick ) const;

	double getTimeInMilliseconds( bpm_t beatsPerMinute ) const;

	static MidiTime fromFrames( const f_cnt_t frames, const float framesPerTick );
	static tick_t ticksPerBar();
	static tick_t ticksPerBar( const TimeSig &sig );
	static int stepsPerBar();
	static void setTicksPerBar( tick_t tpt );
	static MidiTime stepPosition( int step );
	static double ticksToMilliseconds( tick_t ticks, bpm_t beatsPerMinute );
	static double ticksToMilliseconds( double ticks, bpm_t beatsPerMinute );

private:
	tick_t m_ticks;

	static tick_t s_ticksPerBar;

} ;


#endif
