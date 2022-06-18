/*
 * fluidsynthshims.h - a shim header for FluidSynth 2.0 API changes
 *
 * Copyright (c) 2018 Hyunjin Song <tteu.ingog@gmail.com>
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


#ifndef FLUIDSYNTHSHIMS_H
#define FLUIDSYNTHSHIMS_H

#include <fluidsynth.h>

#if FLUIDSYNTH_VERSION_MAJOR < 2

inline const char* fluid_preset_get_name(fluid_preset_t* preset)
{
	return preset->get_name(preset);
}

inline int fluid_preset_get_banknum(fluid_preset_t* preset)
{
	return preset->get_banknum(preset);
}

inline int fluid_preset_get_num(fluid_preset_t* preset)
{
	return preset->get_num(preset);
}

inline fluid_sfont_t* fluid_preset_get_sfont(fluid_preset_t* preset)
{
	return preset->sfont;
}

inline char* fluid_sfont_get_name(fluid_sfont_t* sfont)
{
	return sfont->get_name(sfont);
}

inline void fluid_sfont_iteration_start(fluid_sfont_t* sfont)
{
	sfont->iteration_start(sfont);
}

// Due to the API change, we can't simply shim the 'fluid_sfont_iteration_next' function
inline fluid_preset_t* fluid_sfont_iteration_next_wrapper(fluid_sfont_t* sfont, fluid_preset_t* preset)
{
	return sfont->iteration_next(sfont, preset) ? preset : nullptr;
}

#else // FLUIDSYNTH_VERSION_MAJOR < 2

#define FLUID_REVERB_DEFAULT_ROOMSIZE 0.2f
#define FLUID_REVERB_DEFAULT_DAMP 0.0f
#define FLUID_REVERB_DEFAULT_WIDTH 0.5f
#define FLUID_REVERB_DEFAULT_LEVEL 0.9f

#define FLUID_CHORUS_DEFAULT_N 3
#define FLUID_CHORUS_DEFAULT_LEVEL 2.0f
#define FLUID_CHORUS_DEFAULT_SPEED 0.3f
#define FLUID_CHORUS_DEFAULT_DEPTH 8.0f

inline fluid_preset_t* fluid_sfont_iteration_next_wrapper(fluid_sfont_t* sfont, fluid_preset_t*)
{
	return fluid_sfont_iteration_next(sfont);
}

#endif // FLUIDSYNTH_VERSION_MAJOR < 2

#endif // FLUIDSYNTHSHIMS_H
