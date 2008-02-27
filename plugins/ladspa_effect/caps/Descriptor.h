/*
	Descriptor.h
	
	Copyright 2004-6 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	Creating a LADSPA_Descriptor for a CAPS plugin via a C++ template,
	saves us a virtual function call compared to the usual method used
	for C++ plugins in a C context.

	Descriptor<P> expects P to declare some common methods, like init(),
	activate() etc, plus a static port_info[] and LADSPA_Data * ports[]
	and adding_gain.
 
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

#ifndef _DESCRIPTOR_H_
#define _DESCRIPTOR_H_

#ifdef __SSE__
#include <xmmintrin.h>
#endif
#ifdef __SSE3__
#include <pmmintrin.h>
#endif

/* common stub for Descriptor makes it possible to delete() without special-
 * casing for every plugin class.
 */
class DescriptorStub
: public LADSPA_Descriptor
{
	public:
		static int thishostsucks;

		DescriptorStub()
			{
				PortCount = 0;
			}

		~DescriptorStub()
			{
				if (PortCount)
				{
					delete [] PortNames;
					delete [] PortDescriptors;
					delete [] PortRangeHints;
				}
			}
};

inline void
processor_specific_denormal_measures()
{
	#ifdef __SSE3__
	/* this one works reliably on a 6600 Core2 */
	_MM_SET_DENORMALS_ZERO_MODE (_MM_DENORMALS_ZERO_ON);
	#endif

	#ifdef __SSE__
	/* this one doesn't ... */
	_MM_SET_FLUSH_ZERO_MODE (_MM_FLUSH_ZERO_ON);
	#endif
}

template <class T>
class Descriptor
: public DescriptorStub
{
	public:
		/* Tom Szilyagi reports that hosts exist which call activate() before
		 * connect_port(). Since CAPS' plugins expect ports to be valid we
		 * need a safeguard: at instantiation, each port is connected to the
		 * lower bound. */
		LADSPA_PortRangeHint * ranges;

	public:
		Descriptor() { setup(); }
		void setup();

		void autogen() 
			{
				PortCount = (sizeof (T::port_info) / sizeof (PortInfo));

				/* unroll PortInfo members */
				char ** names = new char * [PortCount];
				LADSPA_PortDescriptor * desc = new LADSPA_PortDescriptor [PortCount];
				ranges = new LADSPA_PortRangeHint [PortCount];

				/* could also assign directly but const_cast is ugly. */
				for (int i = 0; i < (int) PortCount; ++i)
				{
					names[i] = T::port_info[i].name;
					desc[i] = T::port_info[i].descriptor;
					ranges[i] = T::port_info[i].range;
				}

				PortNames = names;
				PortDescriptors = desc;
				PortRangeHints = ranges;
				
				/* LADSPA_Descriptor vtable entries */
				instantiate = _instantiate;
				connect_port = _connect_port;
				activate = _activate;
				run = _run;
				run_adding = _run_adding;
				set_run_adding_gain = _set_run_adding_gain;
				deactivate = 0;
				cleanup = _cleanup;
			}			

		static LADSPA_Handle _instantiate (
				const struct _LADSPA_Descriptor * d, ulong fs)
			{ 
				T * plugin = new T();
				int n = (int) d->PortCount;

				LADSPA_PortRangeHint * ranges = ((Descriptor *) d)->ranges;
				plugin->ranges = ranges;

				plugin->ports = new d_sample * [n];

				/* connect to lower bound as a safety measure */
				for (int i = 0; i < n; ++i)
					plugin->ports[i] = &(ranges[i].LowerBound);

				plugin->fs = fs;
				plugin->normal = NOISE_FLOOR;
				plugin->init();

				return plugin;
			}
		
		static void _connect_port (LADSPA_Handle h, ulong i, LADSPA_Data * p)
			{ 
				((T *) h)->ports[i] = p;
			}

		static void _activate (LADSPA_Handle h)
			{
				T * plugin = (T *) h;

				plugin->first_run = 1;

				/* since none of the plugins do any RT-critical work in 
				 * activate(), it's safe to defer the actual call to the 
				 * plugin's activate() method for the first run() after
				 * the host called in here.
				 * 
				 * It's simplest way to prevent a parameter smoothing sweep
				 * in the first audio block after activation.
				plugin->activate();
				 */
			}

		static void _run (LADSPA_Handle h, ulong n)
			{
				T * plugin = (T *) h;

				/* We don't reset the processor flags later, it's true. */
				processor_specific_denormal_measures();

				/* If this is the first audio block after activation, 
				 * initialize the plugin from the current set of parameters. */
				if (plugin->first_run)
				{
					plugin->activate();
					plugin->first_run = 0;
				}

				plugin->run (n);
				plugin->normal = -plugin->normal;
			}
		
		static void _run_adding (LADSPA_Handle h, ulong n)
			{
				T * plugin = (T *) h;

				/* We don't reset the processor flags later, it's true. */
				processor_specific_denormal_measures();

				/* If this is the first audio block after activation, 
				 * initialize the plugin from the current set of parameters. */
				if (plugin->first_run)
				{
					plugin->activate();
					plugin->first_run = 0;
				}

				plugin->run_adding (n);
				plugin->normal = -plugin->normal;
			}
		
		static void _set_run_adding_gain (LADSPA_Handle h, LADSPA_Data g)
			{
				T * plugin = (T *) h;

				plugin->adding_gain = g;
			}

		static void _cleanup (LADSPA_Handle h)
			{
				T * plugin = (T *) h;

				delete [] plugin->ports;
				delete plugin;
			}
};

#endif /* _DESCRIPTOR_H_ */
