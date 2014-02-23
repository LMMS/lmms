/*
	Clip.h
	
	Copyright 2004-5 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	oversampled hard ('diode', 'transistor', sometimes 'op-amp') clipper.

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

#ifndef _CLIP_H_
#define _CLIP_H_

#include "dsp/util.h"
#include "dsp/FIR.h"
#include "dsp/sinc.h"
#include "dsp/windows.h"

class Clip
: public Plugin
{
	public:
		sample_t gain, gain_db;

		sample_t threshold[2];

		enum {
			OVERSAMPLE = 8,
			FIR_SIZE = 64,
		};

		/* antialias filters */
		DSP::FIRUpsampler up;
		DSP::FIR down;

		template <sample_func_t F>
			void one_cycle (int frames);

		inline sample_t clip (sample_t x);

	public:
		static PortInfo port_info[];

		Clip()
			: up (FIR_SIZE, OVERSAMPLE), 
				down (FIR_SIZE)
			{ }

		void init();

		void activate()
			{
				up.reset();
				down.reset();
				gain_db = *ports[1];
				gain = DSP::db2lin (gain_db);
			}

		void run (int n)
			{
				one_cycle<store_func> (n);
			}
		
		void run_adding (int n)
			{
				one_cycle<adding_func> (n);
			}
};

#endif /* _CLIP_H_ */
