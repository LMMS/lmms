/*
	Eq.cc
	
	Copyright 2002-5 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	10-band octave-spread equalizer.

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

#include "basics.h"
#include <stdio.h>

#include "Eq.h"
#include "Descriptor.h"

void
Eq::init (double _fs)
{
	fs = _fs;
	eq.init (fs, 1.2);
	normal = NOISE_FLOOR;
}

inline static double 
adjust_gain (int i, double g)
{
	static float adjust[] = {
		0.69238604707174034, 0.67282771124180096, 
		0.67215187672467813, 0.65768648447259315, 
		0.65988083755898952, 0.66359580101701909, 
		0.66485139160960427, 0.65890297086039662, 
		0.6493229390740376, 0.82305724539749325
	};

	return g * adjust[i];
}

void
Eq::activate()
{
	for (int i = 0; i < 10; ++i)
	{
		gain[i] = *ports [1 + i];
		eq.gain[i] = adjust_gain (i, DSP::db2lin (gain[i]));
	}
}

template <sample_func_t F>
void
Eq::one_cycle (int frames)
{
	d_sample * s = ports[0];

	/* evaluate band gain changes and compute recursion factor to prevent
	 * zipper noise */
	double one_over_n = 1. / frames;

	for (int i = 0; i < 10; ++i)
	{
		if (*ports [1 + i] == gain[i])
		{
			/* no gain factoring */
			eq.gf[i] = 1;
			continue;
		}

		gain[i] = *ports [1 + i];

		double want = adjust_gain (i, DSP::db2lin (gain[i]));
		eq.gf[i] = pow (want / eq.gain[i], one_over_n);
	}

	d_sample * d = ports[11];

	for (int i = 0; i < frames; ++i)
		F (d, i, eq.process (s[i] + normal), adding_gain);

	normal = -normal;
}

/* //////////////////////////////////////////////////////////////////////// */

PortInfo
Eq::port_info [] =
{
	{
		"in",
		INPUT | AUDIO,
		{0, -1, 1}
	}, {
		"31 Hz",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, -48, 30}
	}, {
		"63 Hz",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, -48, 30}
	}, {
		"125 Hz",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, -48, 30}
	}, {
		"250 Hz",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, -48, 30}
	}, {
		"500 Hz",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, -48, 30}
	}, {
		"1 kHz",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, -48, 30}
	}, {
		"2 kHz",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, -48, 30}
	}, {
		"4 kHz",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, -48, 30}
	}, {
		"8 kHz",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, -48, 30}
	}, {
		"16 kHz",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, -48, 30}
	}, {
		"out",
		OUTPUT | AUDIO,
		{0}
	}
};

template <> void
Descriptor<Eq>::setup()
{
	UniqueID = 1773;
	Label = "Eq";
	Properties = HARD_RT;

	Name = "CAPS: Eq - 10-band 'analogue' equalizer";
	Maker = "Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2004-5";

	/* fill port info and vtable */
	autogen();
}

