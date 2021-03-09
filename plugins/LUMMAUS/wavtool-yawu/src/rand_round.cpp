/*
    wavtool-yawu
    Copyright (C) 2015 StarBrilliant <m13253@hotmail.com>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 3.0 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this program.  If not,
    see <http://www.gnu.org/licenses/>.
*/

#include "rand_round.hpp"
#include <cmath>
#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

namespace YAWU {

#ifdef _WIN32
RandRound::RandRound() :
    fast_random(uint32_t(GetTickCount())) {
}
#else
RandRound::RandRound() :
    fast_random(uint32_t(clock())) {
}
#endif

long RandRound::operator() (double value) {
    double int_part;
    double frac_part = std::modf(value, &int_part);
    float rand_value = fast_random();
    long int_value = long(int_part);
    if(frac_part >= 0) {
        if(frac_part > rand_value)
            int_value++;
    } else {
        if(-frac_part > rand_value)
            int_value--;
    }
    return int_value;
}

}
