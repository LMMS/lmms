/*
	Scape.cc
	
	Copyright 2004-7 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

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

#include "Scape.h"
#include "Descriptor.h"

void
Scape::activate()
{
	time = 0;
	fb = 0;

	for (int i = 0; i < 4; ++i)
		svf[i].reset(),
		svf[i].set_out (SVF::Band),
		hipass[i].set_f (250. / fs);
	svf[3].set_out (SVF::Low),

	delay.reset();
	period = 0;
}

static double 
dividers [] = {
	1 /* 0 sentinel */, 
	1, 0.5, 0.66666666666666666667,	0.75
};

float 
frandom2()
{
	float f = frandom();
	return f * f * f;
}

template <sample_func_t F>
void
Scape::one_cycle (int frames)
{
	sample_t * s = ports[0];

	// double one_over_n = 1 / (double) frames;

	/* delay times */
	double t1 = fs * 60. / getport(1);
	int div = (int) getport(2);
	double t2 = t1 * dividers[div];

	fb = getport(3);

	double dry = getport(4);
	dry = dry * dry;
	double blend = getport(5);

	sample_t * dl = ports[6];
	sample_t * dr = ports[7];

	DSP::FPTruncateMode truncate;

	while (frames)
	{
		/* flip 'renormal' addition constant */
		normal = -normal;

		/* retune filters */
		if (period <= 1)
		{
			period = t2 * .5;
			float f, q;

			f = frandom2();
			svf[0].set_f_Q (300 + 300 * f / fs, .3);
			svf[3].set_f_Q (300 + 600 * 2 * f / fs, .6);
			
			f = frandom2();
			q = f;
			svf[1].set_f_Q (400 + 2400 * f / fs, q);
			q = 1 - f;
			svf[2].set_f_Q (400 + 2400 * f / fs, q);
		}
		
		int n = min ((int) period, frames);
		if (n < 1)
		{
			/* not reached */
			#ifdef DEBUG
			fprintf (stderr, "Scape: %d - %d/%d frames, t2 = %.3f?!?\n", (int) period, n, frames, t2);
			#endif
			return;
		}

		/* sample loop */
		for (int i = 0; i < n; ++i)
		{
			sample_t x = s[i] + normal;

			sample_t x1 = delay.get_at (t1);
			sample_t x2 = delay.get_at (t2);

			delay.put (x + fb * x1 + normal);
			x = dry * x + .2 * svf[0].process (x) + .6 * svf[3].process(x);

			x1 = svf[1].process (x1 - normal);
			x2 = svf[2].process (x2 - normal);

			x1 = hipass[1].process (x1);
			x2 = hipass[2].process (x2);

			sample_t x1l, x1r, x2l, x2r;
			x1l = fabs (lfo[0].get());
			x1r = 1 - x1l;
			x2r = fabs (lfo[1].get());
			x2l = 1 - x2r;

			F (dl, i, x + blend * (x1 * x1l + x2 * x2l), adding_gain);
			F (dr, i, x + blend * (x2 * x2r + x1 * x1r), adding_gain);
		}

		frames -= n;
		period -= n;
		s += n;
		dl += n;
		dr += n;
	}
}

/* //////////////////////////////////////////////////////////////////////// */

PortInfo
Scape::port_info [] =
{
	{
		"in",
		INPUT | AUDIO,
		{BOUNDED, -1, 1}
	}, {
		"bpm",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_LOW, 30, 240}
	}, {
		"divider",
		INPUT | CONTROL,
		{BOUNDED | INTEGER | DEFAULT_MIN, 2, 4}
	}, {
		"feedback",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_LOW, 0, 1}
	}, {
		"dry",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_LOW, 0, 1}
	}, {
		"blend",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_HIGH, 0, 1}
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
Descriptor<Scape>::setup()
{
	UniqueID = 2588;
	Label = "Scape";
	Properties = HARD_RT;

	Name = CAPS "Scape - Stereo delay + Filters";
	Maker = "Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2004-7";

	/* fill port info and vtable */
	autogen();
}

