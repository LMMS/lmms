/*
	SweepVF.cc
	
	Copyright 2002-5 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	SweepVFI, a lorenz fractal modulating the cutoff frequency of a 
	state-variable (ladder) filter.

	SweepVFII, the same with Q being modulated by a second fractal.

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

#include "SweepVF.h"
#include "Descriptor.h"

void
SweepVFI::init (double _fs)
{
	fs = _fs;
	f = .1;
	Q = .1;
	lorenz.init();
	normal = NOISE_FLOOR;
}

void 
SweepVFI::activate()
{ 
	svf.reset();
	svf.set_f_Q (f = *ports[1] / fs, Q = *ports[2]);
}

template <sample_func_t F>
void
SweepVFI::one_cycle (int frames)
{
	d_sample * s = ports[0];

	int blocks = frames / BLOCK_SIZE;
	if (frames & (BLOCK_SIZE - 1))
		++blocks;

	double one_over_blocks = 1 / (double) blocks;
	/* cheesy linear interpolation for f, works well though. */
	double df = (*ports[1] / fs - f) * one_over_blocks;
	double dQ = (*ports[2] - Q) * one_over_blocks;

	svf.set_out ((int) *ports[3]);

	lorenz.set_rate (*ports[7]);

	d_sample * d = ports[8];

	while (frames)
	{
		lorenz.step();

		double modulation = 
				*ports[4] * lorenz.get_x() +
				*ports[5] * lorenz.get_y() +
				*ports[6] * lorenz.get_z();

		double scale = *ports[4] + *ports[5] + *ports[6];

		modulation *= scale * f;
		svf.set_f_Q (max (.001, f + modulation), Q);

		int n = min (frames, BLOCK_SIZE);
		
		for (int i = 0; i < n; ++i)
			F (d, i, svf.process (s[i] + normal), adding_gain);

		s += n;
		d += n;
		frames -= n;

		f += df;
		Q += dQ;
	}

	normal = -normal;

	f = *ports[1] / fs;
	Q = *ports[2];
}

/* //////////////////////////////////////////////////////////////////////// */

PortInfo
SweepVFI::port_info [] =
{
	{
		"in",
		INPUT | AUDIO,
		{0, 0, 0}
	}, {
		"f",
		INPUT | CONTROL,
		{BOUNDED | FS | DEFAULT_LOW, 0.002, 0.08} 
	}, {
		"Q",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_MID, 0.001, .999}
	}, {
		"mode",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_1 | INTEGER, 0, 1} /* only lo and band make sense */
	}, {
		"depth:x",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_LOW, 0, 1}
	}, {
		"depth:y",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_MID, 0, 1}
	}, {
		"depth:z",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_MAX, 0, 1}
	}, {
		"h",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_LOW, 0.001, 1} /* .039 */
	}, {
		"out",
		OUTPUT | AUDIO,
		{0}
	}
};

template <> void
Descriptor<SweepVFI>::setup()
{
	UniqueID = 1782;
	Label = "SweepVFI";
	Properties = HARD_RT;

	Name = "CAPS: SweepVFI - Resonant filter, f swept by a Lorenz fractal";
	Maker = "Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2004-5";

	/* fill port info and vtable */
	autogen();
}

/* //////////////////////////////////////////////////////////////////////// */

void
SweepVFII::init (double _fs)
{
	fs = _fs;
	f = .1;
	Q = .1;
	lorenz1.init();
	lorenz2.init();
	normal = NOISE_FLOOR;
}

void 
SweepVFII::activate()
{ 
	svf.reset();
	svf.set_f_Q (f = *ports[1] / fs, Q = *ports[2]);
}

template <sample_func_t F>
void
SweepVFII::one_cycle (int frames)
{
	d_sample * s = ports[0];

	int blocks = frames / BLOCK_SIZE;
	if (frames & (BLOCK_SIZE - 1))
		++blocks;

	double one_over_blocks = 1 / (double) blocks;
	/* cheesy linear interpolation for f, works well though. */
	double df = (*ports[1] / fs - f) * one_over_blocks;
	double dQ = (*ports[2] - Q) * one_over_blocks;

	svf.set_out ((int) *ports[3]);

	lorenz1.set_rate (*ports[7]);
	lorenz2.set_rate (*ports[11]);

	d_sample * d = ports[12];

	while (frames)
	{
		/* f modulation */
		lorenz1.step();

		double modulation1 = 
				*ports[4] * lorenz1.get_x() +
				*ports[5] * lorenz1.get_y() +
				*ports[6] * lorenz1.get_z();

		double scale1 = *ports[4] + *ports[5] + *ports[6];

		modulation1 *= scale1 * f;

		/* Q modulation */
		lorenz2.step();

		double modulation2 = 
				*ports[8] * lorenz2.get_x() +
				*ports[9] * lorenz2.get_y() +
				*ports[10] * lorenz2.get_z();

		double scale2 = *ports[8] + *ports[9] + *ports[10];

		/* enforce Q limit */
		double q = Q + (modulation2 * scale2 * Q);
		q = min (0.96, max (q, 0));
		
		svf.set_f_Q (max (.001, f + modulation1), q);

		int n = min (frames, BLOCK_SIZE);
		
		for (int i = 0; i < n; ++i)
			F (d, i, svf.process (s[i] + normal), adding_gain);

		s += n;
		d += n;
		frames -= n;

		f += df;
		Q += dQ;
	}

	normal = -normal;

	f = *ports[1] / fs;
	Q = *ports[2];
}

/* //////////////////////////////////////////////////////////////////////// */

PortInfo
SweepVFII::port_info [] =
{
	{
		"in",
		INPUT | AUDIO,
		{0, 0, 0}
	}, {
		"f",
		INPUT | CONTROL,
		{BOUNDED | FS | DEFAULT_LOW, 0.002, 0.08} 
	}, {
		"Q",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_MID, 0.001, .999}
	}, {
		"mode",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_1 | INTEGER, 0, 1} /* only lo and band make sense */
	}, {
		"f:depth:x",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_LOW, 0, 1}
	}, {
		"f:depth:y",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_MID, 0, 1}
	}, {
		"f:depth:z",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_MAX, 0, 1}
	}, {
		"f:h",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_LOW, 0.001, 1} /* .039 */
	}, {
		"Q:depth:x",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_LOW, 0, 1}
	}, {
		"Q:depth:y",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_MID, 0, 1}
	}, {
		"Q:depth:z",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_MAX, 0, 1}
	}, {
		"Q:h",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_LOW, 0.001, 1} /* .039 */
	}, {
		"out",
		OUTPUT | AUDIO,
		{0}
	}
};

template <> void
Descriptor<SweepVFII>::setup()
{
	UniqueID = 2582;
	Label = "SweepVFII";
	Properties = HARD_RT;

	Name = "CAPS: SweepVFII - Resonant filter, f and Q swept by a Lorenz fractal";
	Maker = "Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2004-5";

	/* fill port info and vtable */
	autogen();
}

