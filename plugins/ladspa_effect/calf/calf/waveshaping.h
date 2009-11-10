/* Calf DSP Library
 * Placeholder for waveshaping classes
 *
 * Copyright (C) 2001-2009 Krzysztof Foltman
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA 02111-1307, USA.
 */
#ifndef __CALF_WAVESHAPING_H
#define __CALF_WAVESHAPING_H

/// This will be a waveshaper... when I'll code it (-:
/// (or get Tom Szlagyi's permission to use his own)
class waveshaper {
public:
    waveshaper();
    void activate() {}
    void deactivate() {}
    void set_params(float blend, float drive) {}
    void set_sample_rate(uint32_t sr) {}
    float process(float in) { return in; }
    float get_distortion_level() { return 1; }
};

#endif
