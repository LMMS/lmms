/*
	SweepVF.h
	
	Copyright 2004-7 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	SweepVFI, a lorenz fractal modulating the cutoff frequency of a 
	state-variable (ladder) filter.

	SweepVFII, the same with Q being modulated by a second fractal.

	AutoWah, SVF being modulated by 'instant' amplitude (envelope).

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

#include "dsp/RMS.h"
#include "dsp/BiQuad.h"
#include "dsp/OnePole.h"

class SweepVFI
: public Plugin
{
	public:
		double fs;

		/* svf parameters */
		sample_t f, Q;

		/* needs to be a power of two */
		enum {
			BLOCK_SIZE = 32
		};

		DSP::StackedSVF<1,2> svf;
		DSP::Lorenz lorenz;

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

class SweepVFII
: public Plugin
{
	public:
		/* svf parameters */
		sample_t f, Q;

		/* needs to be a power of two */
		enum {
			BLOCK_SIZE = 32
		};

		DSP::StackedSVF<1,2> svf;
		DSP::Lorenz lorenz1;
		DSP::Lorenz lorenz2;

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

/* //////////////////////////////////////////////////////////////////////// */

class AutoWah
: public Plugin
{
	public:
		double fs;

		/* svf parameters */
		sample_t f, Q;

		/* needs to be a power of two */
		enum {
			BLOCK_SIZE = 32
		};

		DSP::StackedSVF<1,2> svf;
		DSP::RMS rms;

		DSP::BiQuad filter;
		DSP::OnePoleHP hp;

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

#endif /* _SWEEP_VF_H_ */
