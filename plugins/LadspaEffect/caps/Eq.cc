/*
	Eq.cc
	
	Copyright 2002-7 Tim Goetze <tim@quitte.de>
	
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

/* slight adjustments to gain to keep response optimally flat at
 * 0 dB gain in all bands */
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

#define Q 1.414

void
Eq::init()
{
	eq.init (fs, Q); 
}

void
Eq::activate()
{
	for (int i = 0; i < 10; ++i)
	{
		gain[i] = getport (1 + i);
		eq.gain[i] = adjust_gain (i, DSP::db2lin (gain[i]));
		eq.gf[i] = 1;
	}
}

template <sample_func_t F>
void
Eq::one_cycle (int frames)
{
	sample_t * s = ports[0];

	/* evaluate band gain changes and compute recursion factor to prevent
	 * zipper noise */
	double one_over_n = frames > 0 ? 1. / frames : 1;

	for (int i = 0; i < 10; ++i)
	{
		sample_t g = getport (1 + i);
		if (g == gain[i])
		{
			/* no gain factoring */
			eq.gf[i] = 1;
			continue;
		}
		gain[i] = g;

		double want = adjust_gain (i, DSP::db2lin (g));
		eq.gf[i] = pow (want / eq.gain[i], one_over_n);
	}

	sample_t * d = ports[11];

	for (int i = 0; i < frames; ++i)
	{
		sample_t x = s[i];
		x = eq.process (x);
		F (d, i, x, adding_gain);
	}

	eq.normal = -normal;
	eq.flush_0();
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
		{BOUNDED | DEFAULT_0, -48, 24}
	}, {
		"63 Hz",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, -48, 24}
	}, {
		"125 Hz",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, -48, 24}
	}, {
		"250 Hz",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, -48, 24}
	}, {
		"500 Hz",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, -48, 24}
	}, {
		"1 kHz",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, -48, 24}
	}, {
		"2 kHz",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, -48, 24}
	}, {
		"4 kHz",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, -48, 24}
	}, {
		"8 kHz",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, -48, 24}
	}, {
		"16 kHz",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, -48, 24}
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

	Name = CAPS "Eq - 10-band equalizer";
	Maker = "Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2004-7";

	/* fill port info and vtable */
	autogen();
}

/* //////////////////////////////////////////////////////////////////////// */

void
Eq2x2::init()
{
	for (int c = 0; c < 2; ++c)
		eq[c].init (fs, Q);
}

void
Eq2x2::activate()
{
	/* Fetch current parameter settings so we won't sweep band gains in the
	 * first block to process.
	 */
	for (int i = 0; i < 10; ++i)
	{
		gain[i] = getport (2 + i);
		double a = adjust_gain (i, DSP::db2lin (gain[i]));
		for (int c = 0; c < 2; ++c)
			eq[c].gf[i] = 1,
			eq[c].gain[i] = a;
	}
}

template <sample_func_t F>
void
Eq2x2::one_cycle (int frames)
{
	/* evaluate band gain changes and compute recursion factor to prevent
	 * zipper noise */
	double one_over_n = frames > 0 ? 1. / frames : 1;

	for (int i = 0; i < 10; ++i)
	{
		double a;

		if (*ports [2 + i] == gain[i])
			/* still same value, no gain fade */
			a = 1;
		else
		{
			gain[i] = getport (2 + i);
			
			/* prepare factor for logarithmic gain fade */
			a = adjust_gain (i, DSP::db2lin (gain[i]));
			a = pow (a / eq[0].gain[i], one_over_n);
		}

		for (int c = 0; c < 2; ++c)
			eq[c].gf[i] = a;
	}

	for (int c = 0; c < 2; ++c)
	{
		sample_t 
			* s = ports[c],
			* d = ports[12 + c];

		for (int i = 0; i < frames; ++i)
		{
			sample_t x = s[i];
			x = eq[c].process (x);
			F (d, i, x, adding_gain);
		}
	}

	/* flip 'renormal' values */
	for (int c = 0; c < 2; ++c)
	{
		eq[c].normal = normal;
		eq[c].flush_0();
	}
}

PortInfo
Eq2x2::port_info [] =
{
	{
		"in:l",
		INPUT | AUDIO,
		{0, -1, 1}
	}, {
		"in:r",
		INPUT | AUDIO,
		{0, -1, 1}
	}, {
		"31 Hz",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, -48, 24}
	}, {
		"63 Hz",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, -48, 24}
	}, {
		"125 Hz",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, -48, 24}
	}, {
		"250 Hz",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, -48, 24}
	}, {
		"500 Hz",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, -48, 24}
	}, {
		"1 kHz",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, -48, 24}
	}, {
		"2 kHz",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, -48, 24}
	}, {
		"4 kHz",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, -48, 24}
	}, {
		"8 kHz",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, -48, 24}
	}, {
		"16 kHz",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, -48, 24}
	}, {
		"out:l",
		OUTPUT | AUDIO,
		{0}
	}, {
		"out:r",
		OUTPUT | AUDIO,
		{0}
	}
};

template <> void
Descriptor<Eq2x2>::setup()
{
	UniqueID = 2594;
	Label = "Eq2x2";
	Properties = HARD_RT;

	Name = CAPS "Eq2x2 - stereo 10-band equalizer";
	Maker = "Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2004-7";

	/* fill port info and vtable */
	autogen();
}

/* //////////////////////////////////////////////////////////////////////// */

/*
	todo: parametric -- 20-400, 60-1k, 150-2.5k, 500-8k, 1k-20k 
	bandwidth 0-2 octaves
 */
