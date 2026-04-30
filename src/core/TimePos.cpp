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

TimePos::TimePos( const bar_t bar, const tick_t ticks ) :
	m_ticks( bar * s_ticksPerBar + ticks )
{
}

TimePos::TimePos( const tick_t ticks ) :
	m_ticks( ticks )
{
}

TimePos TimePos::quantize(float bars, bool forceRoundDown) const
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
	int snapUp = forceRoundDown || ((2 * offset) == -interval)
		? 0
		: (2 * offset) / interval;

	return (lowPos + snapUp) * interval;
}

} // namespace lmms
