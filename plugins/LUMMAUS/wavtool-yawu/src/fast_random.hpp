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

#ifndef YAWU_FAST_RANDOM_HPP
#define YAWU_FAST_RANDOM_HPP

#include <cstdint>

namespace YAWU {

/**
 * Return a pseudo-random between [0, 1)
 */
class FastRandom {
public:
    FastRandom(uint32_t seed = 1) :
        mirand(seed) {}
    float operator() () {
        mirand *= 16807;
        union { 
            uint32_t i;
            float f;
        } a;
        a.i = (mirand & 0x7fffff) | 0x3f800000;
        return a.f-1;
    }
private:
    uint32_t mirand;
};

}

#endif
