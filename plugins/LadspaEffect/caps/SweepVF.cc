/*
	SweepVF.cc
	
	Copyright 2002-7 Tim Goetze <tim@quitte.de>
	
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

#include <algorithm>

#include "basics.h"

#include "SweepVF.h"
#include "Descriptor.h"

#include "dsp/RBJ.h"

void
SweepVFI::init()
{
	f = .1;
	Q = .1;
	lorenz.init();
}

void 
SweepVFI::activate()
{ 
	svf.reset();
	svf.set_f_Q (f = getport(1) / fs, Q = getport(2));
}

template <sample_func_t F>
void
SweepVFI::one_cycle (int frames)
{
	sample_t * s = ports[0];

	int blocks = frames / BLOCK_SIZE;
	if (frames & (BLOCK_SIZE - 1))
		++blocks;

	double one_over_blocks = 1 / (double) blocks;
	/* cheesy linear interpolation for f, works well though. */
	double df = (getport(1) / fs - f) * one_over_blocks;
	double dQ = (getport(2) - Q) * one_over_blocks;

	svf.set_out ((int) getport(3));

	lorenz.set_rate (getport(7));

	sample_t * d = ports[8];

	while (frames)
	{
		lorenz.step();

		double modulation = 
				getport(4) * lorenz.get_x() +
				getport(5) * lorenz.get_y() +
				getport(6) * lorenz.get_z();

		double scale = getport(4) + getport(5) + getport(6);

		modulation *= scale * f;
		svf.set_f_Q (max (.001, f + modulation), Q);

		int n = std::min<int> (frames, BLOCK_SIZE);
		
		for (int i = 0; i < n; ++i)
			F (d, i, svf.process (s[i] + normal), adding_gain);

		s += n;
		d += n;
		frames -= n;

		f += df;
		Q += dQ;
	}

	f = getport(1) / fs;
	Q = getport(2);
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
		{BOUNDED | LOG | DEFAULT_LOW, 83, 3383} 
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

	Name = CAPS "SweepVFI - Resonant filter swept by a Lorenz fractal";
	Maker = "Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2004-7";

	/* fill port info and vtable */
	autogen();
}

/* //////////////////////////////////////////////////////////////////////// */

void
SweepVFII::init()
{
	f = .1;
	Q = .1;
	lorenz1.init();
	lorenz2.init();
}

void 
SweepVFII::activate()
{ 
	svf.reset();
	svf.set_f_Q (f = getport(1) / fs, Q = getport(2));
}

template <sample_func_t F>
void
SweepVFII::one_cycle (int frames)
{
	sample_t * s = ports[0];

	int blocks = frames / BLOCK_SIZE;
	if (frames & (BLOCK_SIZE - 1))
		++blocks;

	double one_over_blocks = 1 / (double) blocks;
	/* cheesy linear interpolation for f, works well though. */
	double df = (getport(1) / fs - f) * one_over_blocks;
	double dQ = (getport(2) - Q) * one_over_blocks;

	svf.set_out ((int) getport(3));

	lorenz1.set_rate (getport(7));
	lorenz2.set_rate (getport(11));

	sample_t * d = ports[12];

	while (frames)
	{
		/* f modulation */
		lorenz1.step();

		double modulation1 = 
				getport(4) * lorenz1.get_x() +
				getport(5) * lorenz1.get_y() +
				getport(6) * lorenz1.get_z();

		double scale1 = getport(4) + getport(5) + getport(6);

		modulation1 *= scale1 * f;

		/* Q modulation */
		lorenz2.step();

		double modulation2 = 
				getport(8) * lorenz2.get_x() +
				getport(9) * lorenz2.get_y() +
				getport(10) * lorenz2.get_z();

		double scale2 = getport(8) + getport(9) + getport(10);

		/* enforce Q limit */
		double q = Q + (modulation2 * scale2 * Q);
		q = min (0.96, max (q, 0));
		
		svf.set_f_Q (max (.001, f + modulation1), q);

		int n = std::min<int> (frames, BLOCK_SIZE);
		
		for (int i = 0; i < n; ++i)
			F (d, i, svf.process (s[i] + normal), adding_gain);

		s += n;
		d += n;
		frames -= n;

		f += df;
		Q += dQ;
	}

	f = getport(1) / fs;
	Q = getport(2);
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
		{BOUNDED | LOG | DEFAULT_LOW, 83, 3383} 
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

	Name = CAPS "SweepVFII - Resonant filter, f and Q swept by a Lorenz fractal";
	Maker = "Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2004-7";

	/* fill port info and vtable */
	autogen();
}

/* //////////////////////////////////////////////////////////////////////// */

void
AutoWah::init()
{
	f = 800 / fs;
	Q = .5;
}

void 
AutoWah::activate()
{ 
	svf.reset();
	svf.set_f_Q (f = getport(1) / fs, Q = getport(2));
	svf.set_out (DSP::SVF<1>::Band);

	/* hi-passing input for envelope RMS calculation */
	hp.set_f (250. / fs);

	/* smoothing the envelope at 20 Hz */
	DSP::RBJ::LP (20. * BLOCK_SIZE / fs, .6, filter.a, filter.b);

	rms.reset();
	hp.reset();
	filter.reset();
}

template <sample_func_t F>
void
AutoWah::one_cycle (int frames)
{
	sample_t * s = ports[0];

	int blocks = frames / BLOCK_SIZE;
	if (frames & (BLOCK_SIZE - 1))
		++blocks;

	double one_over_blocks = 1 / (double) blocks;
	/* cheesy linear interpolation for f, works well though. */
	double df = (getport(1) / fs - f) * one_over_blocks;
	double dQ = (getport(2) - Q) * one_over_blocks;

	double scale = getport(3);

	sample_t * d = ports[4];

	while (frames)
	{
		double m = rms.rms();

		m = filter.process (m + normal);

		/* Leaving debug output in your code is cheesy! */
		/*
		static int _turn = 0;
		if (_turn++ % 100 == 0)
			fprintf (stderr, "%.4f\n", m);
		*/

		m *= scale * .08;
		svf.set_f_Q (max (.001, f + m), Q);

		int n = std::min<int> (frames, BLOCK_SIZE);
		
		for (int i = 0; i < n; ++i)
		{
			sample_t x = s[i] + normal;
			/* A stacked SVF in bandpass mode is rather quiet, which is
			 * compensated here */
			F (d, i, 2 * svf.process (x), adding_gain);
			/* for envelope calculation, prefer high f content */
			x = hp.process (x);
			rms.store (x * x);
		}

		s += n;
		d += n;
		frames -= n;

		f += df;
		Q += dQ;

		normal = -normal;
	}

	f = getport(1) / fs;
	Q = getport(2);
}

/* //////////////////////////////////////////////////////////////////////// */

PortInfo
AutoWah::port_info [] =
{
	{
		"in",
		INPUT | AUDIO,
		{0, 0, 0}
	}, {
		"f",
		INPUT | CONTROL,
		{BOUNDED | LOG | DEFAULT_LOW, 43, 933} 
	}, {
		"Q",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_LOW, 0.001, .999}
	}, {
		"depth",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_MID, 0, 1}
	}, {
		"out",
		OUTPUT | AUDIO,
		{0}
	}
};

template <> void
Descriptor<AutoWah>::setup()
{
	UniqueID = 2593;
	Label = "AutoWah";
	Properties = HARD_RT;

	Name = CAPS "AutoWah - Resonant envelope-following filter";
	Maker = "Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2004-7";

	/* fill port info and vtable */
	autogen();
}


