/*
	Compress.h
	
	Copyright 2004-5 Tim Goetze <tim@quitte.de>, Steve Harris
	
	http://quitte.de/dsp/

	mono compressor.

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

#ifndef _COMPRESS_H_
#define _COMPRESS_H_

#include "dsp/RMS.h"
#include "dsp/util.h"

class Compress
: public Plugin
{
	public:
		double fs;
		d_sample f;

		DSP::RMS rms;
		d_sample sum, amp, env, gain, gain_t;

		int count;

		template <sample_func_t F>
			void one_cycle (int frames);

	public:
		static PortInfo port_info [];

		void init() {}
		void activate()
			{ 
				rms.reset();

				sum = 0;
				count = 0;
				
				amp = 0;
				env = 0;

				gain = 0;
				gain_t = 0;
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

#endif /* _COMPRESS_H_ */
