/*
	Descriptor.h
	
	Copyright 2004-5 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	creating a LADSPA_Descriptor for a caps plugin via a C++ template,
	saves us a virtual function call compared to the usual method used
	for C++ plugins in a C context.

	Descriptor<P> expects P to declare some common methods, like init(),
	activate() etc, plus a static port_info[] and LADSPA_Data * ports[]
	and of course 'adding_gain'.
	
	maintaining both port_info[] and ports[] is a bit of a bitch, but,
	hey, "you only do it once (tm)" .. and then you do it over and over
	again. particularly bothersome is also the necessary unrolling of our
	PortInfo array to fit into LADSPA_Descriptor's inconsequential way of 
	port data structuring, which results in quite a bit of memory holding 
	duplicated data. oh well.
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

/* common stub for Descriptor makes it possible to delete() without special-
 * casing for every plugin class.
 */
class DescriptorStub
: public LADSPA_Descriptor
{
	public:
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
				
template <class T>
class Descriptor
: public DescriptorStub
{
	public:
		/* tom szilyagi reports that hosts exist which call activate() before
		 * connect_port(). since caps' plugins expect ports to be valid we
		 * need a safeguard: at instantiation, each port is connected to the
		 * lower bound. When (If?) LADSPA default values are ever fixed, connecting
		 * to the default will be preferred. */
		LADSPA_PortRangeHint * ranges;

	public:
		Descriptor()
			{ setup(); }

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
				
				/* see comment above at 'ranges' member */
				for (int i = 0; i < (int) d->PortCount; ++i)
					plugin->ports[i] = &((Descriptor *) d)->ranges[i].LowerBound;

				plugin->init (fs);

				return plugin;
			}
		
		static void _connect_port (LADSPA_Handle h, ulong i, LADSPA_Data * p)
			{ 
				((T *) h)->ports[i] = p;
			}

		static void _activate (LADSPA_Handle h)
			{
				((T *) h)->activate();
			}

		static void _run (LADSPA_Handle h, ulong n)
			{
				/* cannot call template here (g++ 2.95), sigh. */
				((T *) h)->run (n);
			}
		
		static void _run_adding (LADSPA_Handle h, ulong n)
			{
				/* cannot call a template here (g++ 2.95), sigh. */
				((T *) h)->run_adding (n);
			}
		
		static void _set_run_adding_gain (LADSPA_Handle h, LADSPA_Data g)
			{
				((T *) h)->adding_gain = g;
			}

		static void _cleanup (LADSPA_Handle h)
			{
				delete (T *) h;
			}
};

#endif /* _DESCRIPTOR_H_ */
