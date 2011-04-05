/*
	VCO.h
	
	Copyright 2004-5 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	an oversampled triangle/saw/square oscillator, and a combination of two
	such oscillators with hard sync.

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

#ifndef _VCO_H_
#define _VCO_H_

#include "dsp/util.h"
#include "dsp/VCO.h"

#include "dsp/FIR.h"
#include "dsp/sinc.h"
#include "dsp/windows.h"

class VCOs
: public Plugin
{
	public:
		sample_t f, gain;

		/* ok to just change these as you please, 4/32 works ok, sortof. */
		enum {
			OVERSAMPLE = 8,
			FIR_SIZE = 64, 
		};

		DSP::TriSawSquare vco;

		/* downsampling filter */
		DSP::FIR down;

		template <sample_func_t F>
			void one_cycle (int frames);

	public:
		static PortInfo port_info[];

		VCOs()
			: down (FIR_SIZE)
			{ }

		void init();
		void activate()
			{
				gain = *ports[3];
				down.reset();
				vco.reset();
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

/* //////////////////////////////////////////////////////////////////////// */

class VCOd
: public Plugin
{
	public:
		double fs;
		sample_t f, gain;

		/* ok to just change these as you please, 4/32 works ok, sortof. */
		enum {
			OVERSAMPLE = 8,
			FIR_SIZE = 64, 
		};

		DSP::VCO2 vco;

		/* downsampling filter */
		DSP::FIR down;

		template <sample_func_t F>
		void one_cycle (int frames);

	public:
		static PortInfo port_info[];

		VCOd()
			: down (FIR_SIZE)
			{ }

		void init();
		void activate()
			{
				gain = *ports[8];
				down.reset();
				vco.reset();
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

#endif /* _VCO_H_ */
