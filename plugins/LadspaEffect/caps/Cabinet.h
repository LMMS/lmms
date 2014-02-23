/*
	Cabinet.h
	
	Copyright 2002-5 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	CabinetI - 16th order IIR filters modeled after various impulse responses 
	from Steve Harris' 'imp' plugin. Limited to 44.1 kHz sample rate.

	CabinetII - 32nd order IIR filters modeled after the same impulse responses
	using a different algorithm. Versions for 44.1 / 48 / 88.2 / 96 kHz sample
	rates, switched at runtime.
*/
/*
	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
	02111-1307, USA or point your web browser to http://www.gnu.org.
*/

#ifndef _CABINET_H_
#define _CABINET_H_

#include "dsp/util.h"

/* cabinet_float sets the data type used for the IIR history and thus the 
 * computing precision. doubles tend to make the sound more vivid and lively.
 * You can squeeze out a few extra cycles by making this 'float' if needed. 
 * Be warned though that CabinetII has not been tested with 32-bit floats and
 * might become unstable due to the lower computing precision. */
typedef double cabinet_float;

typedef struct {
	int n;
	cabinet_float a[16], b[16];
	float gain;
} Model16;

typedef struct {
	int n;
	cabinet_float a[32], b[32];
	float gain;
} Model32;

class CabinetI
: public Plugin
{
	public:
		sample_t gain;
		static Model16 models [];

		int model;
		void switch_model (int m);

		int n, h;
		cabinet_float * a, * b;
		cabinet_float x[16], y[16];
		
		template <sample_func_t F>
			void one_cycle (int frames);

	public:
		static PortInfo port_info [];

		void init();

		void activate();

		void run (int n)
			{
				one_cycle<store_func> (n);
			}
		
		void run_adding (int n)
			{
				one_cycle<adding_func> (n);
			}
};

/* Second version with 32nd order filters precalculated for
 * 44.1 / 48 / 88.2 / 96 kHz sample rates */

class CabinetII
: public Plugin
{
	public:
		sample_t gain;

		static Model32 models44100 [];
		static Model32 models48000 [];
		static Model32 models88200 [];
		static Model32 models96000 [];

		Model32 * models;
		int model;
		void switch_model (int m);

		int n, h;
		cabinet_float * a, * b;
		cabinet_float x[32], y[32];
		
		template <sample_func_t F>
			void one_cycle (int frames);

	public:
		static PortInfo port_info [];

		sample_t adding_gain;

		void init();
		void activate();

		void run (int n)
			{
				one_cycle<store_func> (n);
			}
		
		void run_adding (int n)
			{
				one_cycle<adding_func> (n);
			}
};

#endif /* _CABINET_H_ */
