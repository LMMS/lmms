/*
	ToneStack.h
	
	Copyright 2006-7
		David Yeh <dtyeh@ccrma.stanford.edu> 
		Tim Goetze <tim@quitte.de> (cosmetics)
	
	Tone Stack emulation.

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

#ifndef _TONESTACK_H_
#define _TONESTACK_H_

#include "dsp/util.h"
#include "dsp/windows.h"
#include "dsp/ToneStack.h"

class ToneStack 
: public Plugin
{
	private:
		DSP::ToneStack tonestack;

		template <sample_func_t F>
			void one_cycle (int frames);

	public:
		static PortInfo port_info [];

		void init()
			{ 
				tonestack.init (fs);
			}

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

/* /////////////////////////////////////////////////////////////////////// */

class ToneStackLT
: public Plugin
{
	private:
		DSP::ToneStackLT tonestack;

		template <sample_func_t F>
			void one_cycle (int frames);

	public:
		static PortInfo port_info [];

		void init()
			{ 
				tonestack.init (fs);
			}

		void activate()
			{ tonestack.activate (ports + 1); }

		void run (int n)
			{
				one_cycle<store_func> (n);
			}
		
		void run_adding (int n)
			{
				one_cycle<adding_func> (n);
			}
};

#endif /* _TONESTACK_H_ */
