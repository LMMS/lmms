/*
	Reverb.cc
	
	Copyright 2002-7 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	three reverb units: JVRev, Plate and Plate2x2.
	
	the former is a rewrite of STK's JVRev, a traditional design.
	
	original comment:
	
		This is based on some of the famous    
		Stanford CCRMA reverbs (NRev, KipRev)  
		all based on the Chowning/Moorer/      
		Schroeder reverberators, which use     
		networks of simple allpass and comb    
		delay filters.  

	the algorithm is mostly unchanged in this implementation; the delay
	line lengths have been fiddled with to make the stereo field more
	evenly weighted, and denormal protection has been added.

	the latter two are based on the circuit discussed in Jon Dattorro's 
	september 1997 JAES paper on effect design (part 1: reverb & filters).
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

#include "Reverb.h"
#include "Descriptor.h"

int 
JVRev::default_length[9] = {
#if 1 /* slightly modified, tg */
	1777, 1847, 1993, 2137, 389, 127, 43, 211, 209
#else
	4799, 4999, 5399, 5801, 1051, 337, 113, 573, 487
#endif
};

void
JVRev::init()
{
	memcpy (length, default_length, sizeof (length));

	if (fs != 44100)
	{
		double s = fs / 44100.;

		for (int i = 0; i < 9; ++i)
		{
			int v = (int) (s * length[i]);
			v |= 1;
			while (!DSP::isprime (v))
				v += 2;
			length[i] = v;
		}
	}
	
	for (int i = 0; i < 4; ++i)
		comb[i].init (length[i]);

	for (int i = 0; i < 3; ++i)
		allpass[i].init (length[i+4]);

	left.init (length[7]);
	right.init (length[8]);

	/* such a simple number, but i couldn't find a better one. */
	apc = .7;
}

void
JVRev::set_t60 (d_sample t)
{
	t60 = t;

	t = max (.00001, t);

	for (int i = 0; i < 4; ++i)
		comb[i].c = pow (10, (-3 * length[i] / (t * fs)));
}

void
JVRev::activate()
{
	for (int i = 0; i < 3; ++i)
		allpass[i].reset();
	
	for (int i = 0; i < 4; ++i)
		comb[i].reset();

	left.reset();
	right.reset();

	set_t60 (getport(1));
}

template <sample_func_t F>
void
JVRev::one_cycle (int frames)
{
	d_sample * s = ports[0];

	if (t60 != *ports[1])
		set_t60 (getport(1));

	double wet = getport(2), dry = 1 - wet;
	
	d_sample * dl = ports[3];
	d_sample * dr = ports[4];

	for (int i = 0; i < frames; ++i)
	{
		d_sample x = s[i], a = x + normal;

		x *= dry;

		/* diffusors */
		a = allpass[0].process (a, -apc);
		a = allpass[1].process (a, -apc);
		a = allpass[2].process (a, -apc);

		/* tank */
		d_sample t = 0;
		a -= normal;

		for (int j = 0; j < 4; ++j)
			t += comb[j].process (a);

		F (dl, i, x + wet * left.putget (t), adding_gain);
		F (dr, i, x + wet * right.putget (t), adding_gain);
	}
}

/* //////////////////////////////////////////////////////////////////////// */

PortInfo
JVRev::port_info [] =
{
	{
		"in",
		INPUT | AUDIO,
		{BOUNDED, -1, 1}
	}, {
		"t60 (s)",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_MID, 0, 4.6}
	}, {
		"blend",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_LOW, 0, .28} 
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
Descriptor<JVRev>::setup()
{
	UniqueID = 1778;
	Label = "JVRev";
	Properties = HARD_RT;

	Name = CAPS "JVRev - Stanford-style reverb from STK";
	Maker = "Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2004-7";

	/* fill port info and vtable */
	autogen();
}

/* //////////////////////////////////////////////////////////////////////// */

void
PlateStub::init()
{
	f_lfo = -1; 
	
#	define L(i) ((int) (l[i] * fs))
	static float l[] = {
		0.004771345048889486, 0.0035953092974026408, 
		0.01273478713752898, 0.0093074829474816042, 
		0.022579886428547427, 0.030509727495715868, 
		0.14962534861059779, 0.060481838647894894, 0.12499579987231611, 
		0.14169550754342933, 0.089244313027116023, 0.10628003091293972
	};

	/* lh */
	input.lattice[0].init (L(0));
	input.lattice[1].init (L(1));
	
	/* rh */
	input.lattice[2].init (L(2));
	input.lattice[3].init (L(3));

	/* modulated, width about 12 samples @ 44.1 */
	tank.mlattice[0].init (L(4), (int) (0.00040322707570310132 * fs));
	tank.mlattice[1].init (L(5), (int) (0.00040322707570310132 * fs));

	/* lh */
	tank.delay[0].init (L(6));
	tank.lattice[0].init (L(7));
	tank.delay[1].init (L(8));

	/* rh */
	tank.delay[2].init (L(9));
	tank.lattice[1].init (L(10));
	tank.delay[3].init (L(11));
#	undef L

#	define T(i) ((int) (t[i] * fs))
	static float t[] = {
		0.0089378717113000241, 0.099929437854910791, 0.064278754074123853, 
		0.067067638856221232, 0.066866032727394914, 0.006283391015086859, 
		0.01186116057928161, 0.12187090487550822, 0.041262054366452743, 
		0.089815530392123921, 0.070931756325392295, 0.011256342192802662
	};

	for (int i = 0; i < 12; ++i)
		tank.taps[i] = T(i);
#	undef T
	
	/* tuned for soft attack, ambience */
	indiff1 = .742;
	indiff2 = .712;

	dediff1 = .723;
	dediff2 = .729;
}

inline void
PlateStub::process (d_sample x, d_sample decay, d_sample * _xl, d_sample * _xr)
{
	x = input.bandwidth.process (x);
	
	/* lh */
	x = input.lattice[0].process (x, indiff1);
	x = input.lattice[1].process (x, indiff1);
	
	/* rh */
	x = input.lattice[2].process (x, indiff2);
	x = input.lattice[3].process (x, indiff2);

	/* summation point */
	register d_sample xl = x + decay * tank.delay[3].get();
	register d_sample xr = x + decay * tank.delay[1].get();

	/* lh */
	xl = tank.mlattice[0].process (xl, dediff1);
	xl = tank.delay[0].putget (xl);
	xl = tank.damping[0].process (xl);
	xl *= decay;
	xl = tank.lattice[0].process (xl, dediff2);
	tank.delay[1].put (xl);

	/* rh */
	xr = tank.mlattice[1].process (xr, dediff1);
	xr = tank.delay[2].putget (xr);
	xr = tank.damping[1].process (xr);
	xr *= decay;
	xr = tank.lattice[1].process (xr, dediff2);
	tank.delay[3].put (xr);

	/* gather output */
	xl  = .6 * tank.delay[2] [tank.taps[0]];
	xl += .6 * tank.delay[2] [tank.taps[1]];
	xl -= .6 * tank.lattice[1] [tank.taps[2]];
	xl += .6 * tank.delay[3] [tank.taps[3]];
	xl -= .6 * tank.delay[0] [tank.taps[4]];
	xl += .6 * tank.lattice[0] [tank.taps[5]];

	xr  = .6 * tank.delay[0] [tank.taps[6]];
	xr += .6 * tank.delay[0] [tank.taps[7]];
	xr -= .6 * tank.lattice[0] [tank.taps[8]];
	xr += .6 * tank.delay[1] [tank.taps[9]];
	xr -= .6 * tank.delay[2] [tank.taps[10]];
	xr += .6 * tank.lattice[1] [tank.taps[11]];

	*_xl = xl;
	*_xr = xr;
}

/* //////////////////////////////////////////////////////////////////////// */

template <sample_func_t F>
void
Plate::one_cycle (int frames)
{
	d_sample * s = ports[0];

	input.bandwidth.set (exp (-M_PI * (1. - getport(1))));

	d_sample decay = getport(2);

	double damp = exp (-M_PI * getport(3));
	tank.damping[0].set (damp);
	tank.damping[1].set (damp);

	d_sample blend = getport(4), dry = 1 - blend;

	d_sample * dl = ports[5];
	d_sample * dr = ports[6];

	/* the modulated lattices interpolate, which needs truncated float */
	DSP::FPTruncateMode _truncate;

	for (int i = 0; i < frames; ++i)
	{
		normal = -normal;
		d_sample x = s[i] + normal;

		d_sample xl, xr;

		PlateStub::process (x, decay, &xl, &xr);

		x = dry * s[i];

		F (dl, i, x + blend * xl, adding_gain);
		F (dr, i, x + blend * xr, adding_gain);
	}
}

/* //////////////////////////////////////////////////////////////////////// */

PortInfo
Plate::port_info [] =
{
	{
		"in",
		INPUT | AUDIO,
		{BOUNDED, -1, 1}
	}, {
		"bandwidth",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_MID, 0.005, .999} /* .9995 */
	}, {
		"tail",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_MID, 0, .749} /* .5 */
	}, {
		"damping",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_LOW, .0005, 1} /* .0005 */
	}, {
		"blend",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_LOW, 0, 1}
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
Descriptor<Plate>::setup()
{
	UniqueID = 1779;
	Label = "Plate";
	Properties = HARD_RT;

	Name = CAPS "Plate - Versatile plate reverb";
	Maker = "Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2004-7";

	/* fill port info and vtable */
	autogen();
}

/* //////////////////////////////////////////////////////////////////////// */

template <sample_func_t F>
void
Plate2x2::one_cycle (int frames)
{
	d_sample * sl = ports[0];
	d_sample * sr = ports[1];

	input.bandwidth.set (exp (-M_PI * (1. - getport(2))));

	d_sample decay = getport(3);

	double damp = exp (-M_PI * getport(4));
	tank.damping[0].set (damp);
	tank.damping[1].set (damp);

	d_sample blend = getport(5), dry = 1 - blend;

	d_sample * dl = ports[6];
	d_sample * dr = ports[7];

	/* the modulated lattices interpolate, which needs truncated float */
	DSP::FPTruncateMode _truncate;

	for (int i = 0; i < frames; ++i)
	{
		normal = -normal;
		d_sample x = (sl[i] + sr[i] + normal) * .5;

		d_sample xl, xr;
		PlateStub::process (x, decay, &xl, &xr);

		xl = blend * xl + dry * sl[i];
		xr = blend * xr + dry * sr[i];

		F (dl, i, xl, adding_gain);
		F (dr, i, xr, adding_gain);
	}
}

/* //////////////////////////////////////////////////////////////////////// */

PortInfo
Plate2x2::port_info [] =
{
	{
		"in:l",
		INPUT | AUDIO,
		{BOUNDED, -1, 1}
	}, {
		"in:r",
		INPUT | AUDIO,
		{BOUNDED, -1, 1}
	}, {
		"bandwidth",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_MID, 0.005, .999} /* .9995 */
	}, {
		"tail",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_MID, 0, .749} /* .5 */
	}, {
		"damping",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_LOW, .0005, 1} /* .0005 */
	}, {
		"blend",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_LOW, 0, 1}
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
Descriptor<Plate2x2>::setup()
{
	UniqueID = 1795;
	Label = "Plate2x2";
	Properties = HARD_RT;

	Name = CAPS "Plate2x2 - Versatile plate reverb, stereo inputs";
	Maker = "Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2004-7";

	/* fill port info and vtable */
	autogen();
}


