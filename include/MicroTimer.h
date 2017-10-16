/*
 * MicroTimer.h - simple high-precision timer
 *
 * Copyright (c) 2005-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef MICRO_TIMER
#define MICRO_TIMER

#include "lmmsconfig.h"

#include <chrono>
#include <cstdlib>
#include "lmms_basics.h"


class MicroTimer
{
	using clock = std::chrono::steady_clock;
	using time_point = std::chrono::steady_clock::time_point;

	static_assert (std::ratio_less_equal<clock::duration::period,
		std::micro>::value, "MicroTimer: steady_clock doesn't support microsecond resolution");

public:
	inline MicroTimer()
	{
		reset();
	}

	inline ~MicroTimer()
	{
	}

	inline void reset()
	{
		begin = clock::now();
	}

	inline int elapsed() const
	{
		auto now = clock::now();
		return std::chrono::duration_cast<std::chrono::duration<int, std::micro>>(now - begin).count();
	}


private:
	time_point begin;
} ;


#endif
