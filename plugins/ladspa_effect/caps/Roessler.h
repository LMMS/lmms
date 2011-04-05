/*
	Roessler.h
	
	Copyright 2004-11 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	turns a Roessler fractal into sound.

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

#ifndef _ROESSLER_H_
#define _ROESSLER_H_

#include "dsp/Roessler.h"

class Roessler
: public Plugin
{
	public:
		sample_t h, gain;

		DSP::Roessler roessler;

		template <sample_func_t F>
		void one_cycle (int frames);

	public:
		static PortInfo port_info [];

		sample_t adding_gain;

		void init();
		void activate()
			{ gain = getport(4); }

		void run (int n)
			{
				one_cycle<store_func> (n);
			}
		
		void run_adding (int n)
			{
				one_cycle<adding_func> (n);
			}
};

#endif /* _ROESSLER_H_ */
