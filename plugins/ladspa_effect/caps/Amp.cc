/*
	Amp.cc
	
	Copyright 2003-7 
		Tim Goetze <tim@quitte.de>
		David Yeh <dtyeh@ccrma.stanford.edu> (Tone Stack in TS models)
	
	http://quitte.de/dsp/

	Tube amplifier models

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
AmpStub::init (bool adjust_downsampler)
{
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
}

/* //////////////////////////////////////////////////////////////////////// */

void
AmpIII::init()
{
	this->AmpStub::init (false);
	
	/* need to filter out dc before the power amp stage, which is running at
	 * the oversampled rate */
	dc_blocker.set_f (10. / (fs * OVERSAMPLE));
	
	DSP::RBJ::LoShelve (200 / fs, .2, -3, filter.a, filter.b);
}

template <sample_func_t F, int OVERSAMPLE>
void
AmpIII::one_cycle (int frames)
{
	sample_t * s = ports[0];
	
	sample_t gain = getport(1);
	sample_t temp = getport(2) * tube.scale;
	
	drive = getport(3) * .5; 
	i_drive = 1 / (1 - drive);
	
	sample_t * d = ports[4];
	
	*ports[5] = OVERSAMPLE;

	double g = current.g;

	current.g = max (gain < 1 ? gain : exp2 (gain - 1), .000001);
	current.g *= tube.scale / fabs (tube.transfer (temp)); 

	/* recursive fade to prevent zipper noise from the 'gain' knob */
	if (g == 0) g = current.g;
	
	double one_over_n = frames > 0 ? 1. / frames : 1;
	double gf = pow (current.g / g, one_over_n);

	for (int i = 0; i < frames; ++i)
	{
		register sample_t a = s[i];

		a = g * tube.transfer (a * temp);
		a = filter.process (a + normal);

		a = tube.transfer_clip (up.upsample (a));
		a = power_transfer (dc_blocker.process (a));
		
		a = down.process (a);

		for (int o = 1; o < OVERSAMPLE; ++o)
			down.store (
					power_transfer (
						dc_blocker.process (
							normal + tube.transfer_clip (up.pad (o)))));

		F (d, i, a, adding_gain);

		g *= gf;
	}

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

	Name = CAPS "AmpIII - Tube amp";
	Maker = "Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2002-7";

	/* fill port info and vtable */
	autogen();
}

/* //////////////////////////////////////////////////////////////////////// */

void
AmpIV::init()
{
	this->AmpStub::init (false);
	
	/* need to filter out dc before the power amp stage, which is running at
	 * the oversampled rate */
	dc_blocker.set_f (10. / (fs * OVERSAMPLE));
	
	tone.init (fs);
}

template <sample_func_t F, int OVERSAMPLE>
void
AmpIV::one_cycle (int frames)
{
	double one_over_n = frames > 0 ? 1. / frames : 1;

	sample_t * s = ports[0];
	
	sample_t gain = getport(1);
	sample_t temp = getport(2) * tube.scale;

	tone.start_cycle (ports + 3, one_over_n);
	
	drive = getport(7) * .5; 
	i_drive = 1 / (1 - drive);
	
	sample_t * d = ports[8];
	
	*ports[9] = OVERSAMPLE;

	double g = current.g;

	current.g = max (gain < 1 ? gain : exp2 (gain - 1), .000001);
	current.g *= tube.scale / fabs (tube.transfer (temp)); 

	/* recursive fade to prevent zipper noise from the 'gain' knob */
	if (g == 0) g = current.g;
	
	double gf = pow (current.g / g, one_over_n);

	for (int i = 0; i < frames; ++i)
	{
		register sample_t a = s[i] + normal;

		a = g * tube.transfer (a * temp);
		a = tone.process (a);

		a = tube.transfer_clip (up.upsample (a));
		a = power_transfer (dc_blocker.process (a));
		
		a = down.process (a);

		for (int o = 1; o < OVERSAMPLE; ++o)
			down.store (
					power_transfer (
						dc_blocker.process (
							normal + tube.transfer_clip (up.pad (o)))));

		F (d, i, a, adding_gain);

		g *= gf;
	}

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

	Name = CAPS "AmpIV - Tube amp + tone controls";
	Maker = "Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2002-7";

	/* fill port info and vtable */
	autogen();
}

/* //////////////////////////////////////////////////////////////////////// */

void
AmpV::init()
{
	this->AmpStub::init (false);
	
	/* need to filter out dc before the power amp stage, which is running at
	 * the oversampled rate */
	dc_blocker.set_f (10. / (fs * OVERSAMPLE));
	
	DSP::RBJ::LoShelve (210. / fs, .2, -1, filter[0].a, filter[0].b);
	DSP::RBJ::LoShelve (4200. / fs, 1.2, +6, filter[1].a, filter[1].b);
	DSP::RBJ::LoShelve (420. / fs, .2, +2, filter[2].a, filter[2].b);

	/* power supply cap */
	for (int i = 0; i < 2; ++i)
		DSP::RBJ::LP (10. / fs, .3, power_cap[i].a, power_cap[i].b);
}

template <sample_func_t F, int OVERSAMPLE>
void
AmpV::one_cycle (int frames)
{
	sample_t * s = ports[0];
	
	sample_t gain = getport(1);

	if (*ports[2] != cut)
	{
		cut = getport(2);
		DSP::RBJ::LoShelve (210. / fs, .2, cut, filter[0].a, filter[0].b);
	}
	if (*ports[3] != tone)
	{
		tone = getport(3);
		double f = tone * tone * 8400 + 420;
		double q = tone * .4 + .2;
		double db = tone * 2 + 2;
		DSP::RBJ::LoShelve (f / fs, q, db, filter[2].a, filter[2].b);
	}

	drive = getport(4) * .5; 
	i_drive = 1 / (1 - drive);
	
	#define MAX_WATTS port_info[5].range.UpperBound
	sample_t sag = (MAX_WATTS - getport(5)) / MAX_WATTS;
	sag = .6 * sag * sag;
		
	sample_t * d = ports[6]; 
	
	*ports[7] = OVERSAMPLE;

	double g = current.g;

	current.g = max (gain < 1 ? gain : pow (20, gain - 1), .000001);
	#if 0
	if (++_turn & 127) == 0)
		fprintf (stderr, "supply = %.3f sag = %.3f\n", supply, sag);
	#endif

	if (g == 0) g = current.g;

	/* recursive fade to prevent zipper noise from the 'gain' knob */
	double one_over_n = frames > 0 ? 1. / frames : 1;
	double gf = pow (current.g / g, one_over_n);

	for (int i = 0; i < frames; ++i)
	{
		register sample_t a = s[i];
		register sample_t v = 3 - supply;
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
								normal + tube.transfer_clip (
									up.pad (o)))));
		}

		F (d, i, a, adding_gain);
		
		/* integrate for an approximation of cumulative output power */
		supply += sag * fabs (a) + normal;
		/* filter integrated power consumption */
		for (int j = 0; j < 2; ++j)
			supply = 0.9 * (power_cap[j].process (supply));

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

	Name = CAPS "AmpV - Tube amp";
	Maker = "Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2002-7";

	/* fill port info and vtable */
	autogen();
}

/* //////////////////////////////////////////////////////////////////////// */

void
AmpVTS::init()
{
	this->AmpStub::init (false);
	
	/* need to filter out dc before the power amp stage, which is running at
	 * the oversampled rate */
	dc_blocker.set_f (10. / (fs * OVERSAMPLE));
	
	/* power supply capacitance */
	for (int i = 0; i < 2; ++i)
		DSP::RBJ::LP (10. / fs, .3, power_cap[i].a, power_cap[i].b);

	tonestack.init (fs);
}

template <sample_func_t F, int OVERSAMPLE>
void
AmpVTS::one_cycle (int frames)
{
	sample_t * s = ports[0];

	tonestack.start_cycle (ports + 1, 2);
	sample_t gain = getport(2);

	drive = getport(6) * .5; 
	i_drive = 1 / (1 - drive);
	
	sample_t sag = 1 - max (0.0001, min (1, getport(7)));
	sag = .6 * sag * sag; /* map to log space makes slider better */
		
	sample_t * d = ports[8]; 
	
	*ports[9] = OVERSAMPLE;

	double g = current.g;

	if (gain < 1)
		current.g = max (gain, .001);
	else
	{
		gain -= 1;
		gain *= gain;
		current.g = pow (10, gain);
	}

	/* recursive fade to prevent zipper noise from the 'gain' knob */
	double one_over_n = frames > 0 ? 1. / frames : 1;
	double gf = pow (current.g / g, one_over_n);

	for (int i = 0; i < frames; ++i)
	{
		register double a = s[i];
		register double v = 3 - supply;
		v = v * v * .06 + .46;

		a = tube.transfer (a);
		a = tonestack.process (a + normal);
		
		a = g * (a + supply * .001); 

		a = v * tube.transfer_clip (up.upsample (a));
		a = power_transfer (dc_blocker.process (a));
		
		a = down.process (a);

		{
			for (int o = 1; o < OVERSAMPLE; ++o)
				down.store (
						power_transfer (
							dc_blocker.process (
								normal + tube.transfer_clip (
									up.pad (o)))));
		}

		F (d, i, a, adding_gain);
		
		/* integrate for an approximation of cumulative output power */
		supply += sag * fabs (a) + normal;
		/* filter integrated power consumption */
		for (int j = 0; j < 2; ++j)
			supply = 0.9 * (power_cap[j].process (supply + normal));

		g *= gf;
		normal = -normal;
	}

	current.g = g;
}

/* //////////////////////////////////////////////////////////////////////// */

PortInfo
AmpVTS::port_info [] = 
{
	{
		"in",
		INPUT | AUDIO,
		{BOUNDED, -1, 1}
	}, {
		"model",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0 | INTEGER, 0, 5} /* no way to set dyn at compile t */
	}, {
		"gain",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_HIGH, 0, 3}
	}, {
		"bass",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_MID, 0, 1}
	}, {
		"mid",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_1, 0, 1}
	}, {
		"treble",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_HIGH, 0, 1}
	}, {
		"drive",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_LOW, 0.0001, 1} 
	}, {
		"watts",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_HIGH, 0.0001, 1}
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
Descriptor<AmpVTS>::setup()
{
	UniqueID = 2592;
	Label = "AmpVTS";
	Properties = HARD_RT;

	Name = CAPS "AmpVTS - Tube amp + Tone stack";
	Maker = "David Yeh <dtyeh@ccrma.stanford.edu> & Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2002-7";

	/* fill port info and vtable */
	autogen();
}
