/*
	Pan.h
	
	Copyright 2004-11 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	panorama with width control, 
	stereo image width reduction

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

#ifndef _PAN_H_
#define _PAN_H_

#include "dsp/Delay.h"
#include "dsp/OnePole.h"

class PanTap
{
	public:
		int t;
		DSP::OnePoleLP damper;

		sample_t get (DSP::Delay & delay)
			{
				return damper.process (delay[t]);
			}

		void reset (double c)
			{
				damper.set_f (c);
				damper.reset();
			}
};

class Pan
: public Plugin
{
	public:
		sample_t pan;

		sample_t gain_l, gain_r;

		DSP::Delay delay;
		PanTap tap;

		template <sample_func_t F>
			void one_cycle (int frames);

		inline void set_pan (sample_t);

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

/* stereo width reduction */
class Narrower
: public Plugin
{
	public:
		sample_t strength;

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

#endif /* _PAN_H_ */
