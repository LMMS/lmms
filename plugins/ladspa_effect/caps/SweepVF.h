/*
	SweepVF.h
	
	Copyright 2004-5 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	SweepVFI, a lorenz fractal modulating the cutoff frequency of a 
	state-variable (ladder) filter.

	SweepVFII, the same with Q being modulated by a second fractal.

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

#ifndef _SWEEP_VF_H_
#define _SWEEP_VF_H_

#include "dsp/SVF.h"
#include "dsp/Lorenz.h"
#include "dsp/Roessler.h"

class SweepVFI
{
	public:
		double fs;

		/* svf parameters */
		d_sample f, Q;

		/* needs to be a power of two */
		enum {
			BLOCK_SIZE = 32
		};

		DSP::StackedSVF<1,2> svf;
		DSP::Lorenz lorenz;

		d_sample normal;

		template <sample_func_t F>
		void one_cycle (int frames);

	public:
		static PortInfo port_info [];
		d_sample * ports [9];

		d_sample adding_gain;

		void init (double _fs);

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

class SweepVFII
{
	public:
		double fs;

		/* svf parameters */
		d_sample f, Q;

		/* needs to be a power of two */
		enum {
			BLOCK_SIZE = 32
		};

		DSP::StackedSVF<1,2> svf;
		DSP::Lorenz lorenz1;
		DSP::Lorenz lorenz2;

		d_sample normal;

		template <sample_func_t F>
		void one_cycle (int frames);

	public:
		static PortInfo port_info [];
		d_sample * ports [13];

		d_sample adding_gain;

		void init (double _fs);

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

#endif /* _SWEEP_VF_H_ */
