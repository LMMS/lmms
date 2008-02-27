/*
	Phaser.h
	
	Copyright 2002-5 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	Standard and fractal-modulated phaser units.

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

#ifndef _PHASER_H_
#define _PHASER_H_

#include "dsp/Sine.h"
#include "dsp/Lorenz.h"
#include "dsp/Delay.h"

/* all-pass as used by the phaser. */
class PhaserAP
{
	public:
		d_sample a, m;
		
		PhaserAP() 
		{ 
			a = m = 0.; 
		}

		void set (double delay)
		{
			a = (1 - delay) / (1 + delay);
		}

		d_sample process (d_sample x)
		{
			register d_sample y = -a * x + m;
			m = a * y + x;

			return y;
		}
};

class PhaserI
: public Plugin
{
	public:
		PhaserAP ap[6];
		DSP::Sine lfo;

		d_sample rate;
		d_sample y0;

		struct {
			double bottom, range;
		} delay;

		template <sample_func_t>
			void one_cycle (int frames);
	
		int blocksize, remain;

	public:
		static PortInfo port_info [];

		void init()
			{
				blocksize = 32;
			}

		void activate()
			{
				y0 = 0.;
				remain = 0;

				delay.bottom = 400. / fs;
				delay.range = 2200. / fs;

				rate = -1; /* force lfo reset in one_cycle() */
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

/* same as above, but filter sweep is controlled by a Lorenz fractal */

class PhaserII
: public Plugin
{
	public:
		double fs;

		PhaserAP ap[6];
		DSP::Lorenz lorenz;

		d_sample rate;
		d_sample y0;

		struct {
			double bottom, range;
		} delay;

		template <sample_func_t>
			void one_cycle (int frames);
	
		int blocksize, remain;

	public:
		static PortInfo port_info [];

		void init()
			{
				blocksize = 32;
				lorenz.init();
			}

		void activate()
			{
				y0 = 0.;
				remain = 0;

				delay.bottom = 400. / fs;
				delay.range = 2200. / fs;

				rate = -1; /* force lfo reset in one_cycle() */
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


#endif /* _PHASER_H_ */
