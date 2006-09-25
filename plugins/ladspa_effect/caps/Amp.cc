/*
	Amp.cc
	
	Copyright 2003-6 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	tube amplifier models
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

#include "Amp.h"
#include "Descriptor.h"

void
AmpStub::init (double _fs, bool adjust_downsampler)
{
	fs = _fs;
	dc_blocker.set_f (10. / fs);

	/* going a bit lower than nominal with fc */
	double f = .7 * M_PI / OVERSAMPLE;
	
	/* construct the upsampler filter kernel */
	DSP::sinc (f, up.c, FIR_SIZE);
	DSP::kaiser<DSP::apply_window> (up.c, FIR_SIZE, 6.4);

	/* copy upsampler filter kernel for downsampler, make sum */
	double s = 0;
	for (int i = 0; i < up.n; ++i)
		down.c[i] = up.c[i],
		s += up.c[i];
	
	s = 1 / s;

	/* scale downsampler kernel for unity gain + correction for transfer */
	double t = adjust_downsampler ? 
		s / max (fabs (tube.clip[0].value), fabs (tube.clip[1].value)) : s;

	for (int i = 0; i < down.n; ++i)
		down.c[i] *= t;

	/* scale upsampler kernel for unity gain */
	s *= OVERSAMPLE;
	for (int i = 0; i < up.n; ++i)
		up.c[i] *= s;

	normal = NOISE_FLOOR;
}

/* //////////////////////////////////////////////////////////////////////// */

void
AmpIII::init (double _fs)
{
	this->AmpStub::init (_fs, false);
	
	/* need to filter out dc before the power amp stage, which is running at
	 * the oversampled rate */
	dc_blocker.set_f (10. / (fs * OVERSAMPLE));
	
	DSP::RBJ::LoShelve (200 / (_fs), .2, -3, filter.a, filter.b);
}

template <sample_func_t F, int OVERSAMPLE>
void
AmpIII::one_cycle (int frames)
{
	d_sample * s = ports[0];
	
	d_sample gain = *ports[1];
	d_sample temp = *ports[2] * tube.scale;
	
	drive = *ports[3] * .5; 
	i_drive = 1 / (1 - drive);
	
	d_sample * d = ports[4];
	
	*ports[5] = OVERSAMPLE;

	double g = current.g;

	current.g = max (gain < 1 ? gain : exp2 (gain - 1), .000001);
	current.g *= tube.scale / fabs (tube.transfer (temp)); 

	/* recursive fade to prevent zipper noise from the 'gain' knob */
	if (g == 0) g = current.g;
	
	double one_over_n = 1. / frames;
	double gf = pow (current.g / g, one_over_n);

	for (int i = 0; i < frames; ++i)
	{
		register d_sample a = s[i];

		a = g * tube.transfer (a * temp);
		a = filter.process (a + normal);

		a = tube.transfer_clip (up.upsample (a));
		a = power_transfer (dc_blocker.process (a));
		
		a = down.process (a);

		for (int o = 1; o < OVERSAMPLE; ++o)
			down.store (
					power_transfer (
						dc_blocker.process (
							tube.transfer_clip (up.pad (o)))));

		F (d, i, a, adding_gain);

		g *= gf;
	}

	normal = -normal;
	current.g = g;
}

/* //////////////////////////////////////////////////////////////////////// */

PortInfo
AmpIII::port_info [] = 
{
	{
		"in",
		INPUT | AUDIO,
		{BOUNDED, -1, 1}
	}, {
		"gain",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_1, 0, 10}
	}, {
		"temperature",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_MID, 0.005, 1}
	}, {
		"drive",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_MAX, 0.0001, 1} /* ^2 gives the nice drive */
	}, {
		"out",
		OUTPUT | AUDIO,
		{0}
	}, {
		"latency",
		OUTPUT | CONTROL,
		{0}
	}
};

template <> void
Descriptor<AmpIII>::setup()
{
	UniqueID = 1786;
	Label = "AmpIII";
	Properties = HARD_RT;

	Name = "CAPS: AmpIII - Tube amp emulation";
	Maker = "Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2002-5";

	/* fill port info and vtable */
	autogen();
}

/* //////////////////////////////////////////////////////////////////////// */

void
AmpIV::init (double _fs)
{
	this->AmpStub::init (_fs, false);
	
	/* need to filter out dc before the power amp stage, which is running at
	 * the oversampled rate */
	dc_blocker.set_f (10. / (fs * OVERSAMPLE));
	
	tone.init (_fs);
}

template <sample_func_t F, int OVERSAMPLE>
void
AmpIV::one_cycle (int frames)
{
	double one_over_n = 1. / frames;

	d_sample * s = ports[0];
	
	d_sample gain = *ports[1];
	d_sample temp = *ports[2] * tube.scale;
	
	tone.start_cycle (ports + 3, one_over_n);
	
	drive = *ports[7] * .5; 
	i_drive = 1 / (1 - drive);
	
	d_sample * d = ports[8];
	
	*ports[9] = OVERSAMPLE;

	double g = current.g;

	current.g = max (gain < 1 ? gain : exp2 (gain - 1), .000001);
	current.g *= tube.scale / fabs (tube.transfer (temp)); 

	/* recursive fade to prevent zipper noise from the 'gain' knob */
	if (g == 0) g = current.g;
	
	double gf = pow (current.g / g, one_over_n);

	for (int i = 0; i < frames; ++i)
	{
		register d_sample a = s[i] + normal;

		a = g * tube.transfer (a * temp);
		a = tone.process (a);

		a = tube.transfer_clip (up.upsample (a));
		a = power_transfer (dc_blocker.process (a));
		
		a = down.process (a);

		for (int o = 1; o < OVERSAMPLE; ++o)
			down.store (
					power_transfer (
						dc_blocker.process (
							tube.transfer_clip (up.pad (o)))));

		F (d, i, a, adding_gain);

		g *= gf;
	}

	normal = -normal;
	current.g = g;
}

/* //////////////////////////////////////////////////////////////////////// */

PortInfo
AmpIV::port_info [] = 
{
	{
		"in",
		INPUT | AUDIO,
		{BOUNDED, -1, 1}
	}, {
		"gain",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_1, 0, 10}
	}, {
		"temperature",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_MID, 0.005, 1}
	}, {
		"bass",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, -20, 20}
	}, {
		"mid",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, -20, 20}
	}, {
		"treble",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, -20, 20}
	}, {
		"hi",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, -20, 20}
	}, {
		"drive",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_MAX, 0.0001, 1} /* ^2 gives the nice drive */
	}, {
		"out",
		OUTPUT | AUDIO,
		{0}
	}, {
		"latency",
		OUTPUT | CONTROL,
		{0}
	}
};

template <> void
Descriptor<AmpIV>::setup()
{
	UniqueID = 1794;
	Label = "AmpIV";
	Properties = HARD_RT;

	Name = "CAPS: AmpIV - Tube amp emulation + tone controls";
	Maker = "Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2002-5";

	/* fill port info and vtable */
	autogen();
}

/* //////////////////////////////////////////////////////////////////////// */

void
AmpV::init (double _fs)
{
	this->AmpStub::init (_fs, false);
	
	/* need to filter out dc before the power amp stage, which is running at
	 * the oversampled rate */
	dc_blocker.set_f (10. / (_fs * OVERSAMPLE));
	
	DSP::RBJ::LoShelve (210. / _fs, .2, -1, filter[0].a, filter[0].b);
	DSP::RBJ::LoShelve (4200. / _fs, 1.2, +6, filter[1].a, filter[1].b);
	DSP::RBJ::LoShelve (420. / _fs, .2, +2, filter[2].a, filter[2].b);

	/* power supply capacitor */
	for (int i = 0; i < 2; ++i)
		DSP::RBJ::LP (10. / _fs, .3, power_cap[i].a, power_cap[i].b);
}

static int _turn = 0;

template <sample_func_t F, int OVERSAMPLE>
void
AmpV::one_cycle (int frames)
{
	d_sample * s = ports[0];
	
	d_sample gain = *ports[1];

	if (*ports[2] != cut)
	{
		cut = *ports[2];
		DSP::RBJ::LoShelve (210. / fs, .2, cut, filter[0].a, filter[0].b);
	}
	if (*ports[3] != tone)
	{
		tone = *ports[3];
		double f = tone * tone * 8400 + 420;
		double q = tone * .4 + .2;
		double db = tone * 2 + 2;
		DSP::RBJ::LoShelve (f / fs, q, db, filter[2].a, filter[2].b);
	}

	drive = *ports[4] * .5; 
	i_drive = 1 / (1 - drive);
	
	#define MAX_WATTS port_info[5].range.UpperBound
	d_sample sag = (MAX_WATTS - *ports[5]) / MAX_WATTS;
	sag = .6 * sag * sag;
		
	d_sample * d = ports[6]; 
	
	*ports[7] = OVERSAMPLE;

	double g = current.g;

	current.g = max (gain < 1 ? gain : pow (20, gain - 1), .000001);
	if (0 && (++_turn & 127) == 0)
		fprintf (stderr, "supply = %.3f sag = %.3f\n", supply, sag);

	if (g == 0) g = current.g;

	/* recursive fade to prevent zipper noise from the 'gain' knob */
	double one_over_n = 1. / frames;
	double gf = pow (current.g / g, one_over_n);

	for (int i = 0; i < frames; ++i)
	{
		register d_sample a = s[i];
		register d_sample v = 3 - supply;
		/* alternative curve: v = v * v * .1 + .1; */
		v = v * v * .06 + .46;

		a = filter[0].process (a + normal);
		if (0)
			a = filter[2].process (a);
		
		a = g * (a + supply * .001); 

		a = v * tube.transfer_clip (up.upsample (a));
		a = power_transfer (dc_blocker.process (a));
		
		a = down.process (a);

		a = filter[1].process (a - normal);
		if (1)
			a = filter[2].process (a + normal);

		{
			for (int o = 1; o < OVERSAMPLE; ++o)
				down.store (
						power_transfer (
							dc_blocker.process (
								tube.transfer_clip (
									up.pad (o)))));
		}

		F (d, i, a, adding_gain);
		
		/* integrate for an approximation of cumulative output power */
		supply += sag * fabs (a) + normal;
		/* filter integrated power consumption */
		for (int i = 0; i < 2; ++i)
			supply = 0.9 * (power_cap[i].process (supply));

		g *= gf;
		normal = -normal;
	}

	current.g = g;
}

/* //////////////////////////////////////////////////////////////////////// */

PortInfo
AmpV::port_info [] = 
{
	{
		"in",
		INPUT | AUDIO,
		{BOUNDED, -1, 1}
	}, {
		"gain",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_1, 0, 3}
	}, {
		"bass",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, -9, 9}
	}, {
		"tone",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_MIN, 0, 1}
	}, {
		"drive",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_HIGH, 0.0001, 1} /* ^2 gives the nice drive */
	}, {
		"watts",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_MID, 5, 150}
	}, {
		"out",
		OUTPUT | AUDIO,
		{0}
	}, {
		"latency",
		OUTPUT | CONTROL,
		{0}
	}
};

template <> void
Descriptor<AmpV>::setup()
{
	UniqueID = 2587;
	Label = "AmpV";
	Properties = HARD_RT;

	Name = "CAPS: AmpV - Refined tube amp emulation";
	Maker = "Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2002-5";

	/* fill port info and vtable */
	autogen();
}


