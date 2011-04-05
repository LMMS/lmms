/*
	Preamp.cc
	
	Copyright 2003-7 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	Loosely 12AX7-based tube preamp model with 8x oversampling.
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
PreampIII::init()
{
	this->AmpStub::init();

	DSP::RBJ::LoShelve (200 / fs, .2, -6, filter.a, filter.b);
}

template <sample_func_t F, int OVERSAMPLE>
void
PreampIII::one_cycle (int frames)
{
	sample_t * s = ports[0];
	sample_t gain = getport(1);
	sample_t temp = getport(2) * tube.scale;
	sample_t * d = ports[3];
	*ports[4] = OVERSAMPLE;

	double g = current.g;

	current.g = max (gain < 1 ? gain : exp2 (gain - 1), .000001);
	/* correction for attenuated first transfer */	
	current.g *= tube.scale / fabs (tube.transfer (temp)); 

	if (g == 0) g = current.g;
	
	/* recursive fade to prevent zipper noise from the 'gain' knob */
	double one_over_n = frames > 0 ? 1. / frames : 1;
	double gf = pow (current.g / g, one_over_n);

	for (int i = 0; i < frames; ++i)
	{
		register sample_t a = s[i] + normal;

		a = g * tube.transfer (a * temp);
		a = filter.process (a);
		a = down.process (tube.transfer_clip (up.upsample (a)));

		for (int o = 1; o < OVERSAMPLE; ++o)
			down.store (tube.transfer_clip (up.pad (o)));

		F (d, i, dc_blocker.process (a), adding_gain);

		g *= gf;
	}

	current.g = g;
}

/* //////////////////////////////////////////////////////////////////////// */

PortInfo
PreampIII::port_info [] = 
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
		{BOUNDED | DEFAULT_LOW, 0.005, 1}
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
Descriptor<PreampIII>::setup()
{
	UniqueID = 1776;
	Label = "PreampIII";
	Properties = HARD_RT;

	Name = CAPS "PreampIII - Tube preamp emulation";
	Maker = "Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2002-7";

	/* fill port info and vtable */
	autogen();
}

/* //////////////////////////////////////////////////////////////////////// */

void
PreampIV::init()
{
	this->AmpStub::init();
	tone.init (fs);
}

void
PreampIV::activate()
{
	this->PreampIII::activate();

	tone.activate (ports + 3);
}

template <sample_func_t F, int OVERSAMPLE>
void
PreampIV::one_cycle (int frames)
{
	double one_over_n = frames > 0 ? 1. / frames : 1;

	sample_t * s = ports[0];
	sample_t gain = getport(1);
	sample_t temp = getport(2) * tube.scale;

	tone.start_cycle (ports + 3, one_over_n);

	sample_t * d = ports[7];
	*ports[8] = OVERSAMPLE;

	double g = current.g;

	current.g = max (gain < 1 ? gain : exp2 (gain - 1), .000001);
	current.g *= tube.scale / fabs (tube.transfer (temp)); 

	if (g == 0) g = current.g;
	
	/* recursive fade to prevent zipper noise from the 'gain' knob */
	double gf = pow (current.g / g, one_over_n);

	for (int i = 0; i < frames; ++i)
	{
		register sample_t a = tone.process (s[i] + normal);

		a = g * tube.transfer (a * temp);

		a = down.process (tube.transfer_clip (up.upsample (a)));

		for (int o = 1; o < OVERSAMPLE; ++o)
			down.store (tube.transfer_clip (up.pad (o)));

		F (d, i, dc_blocker.process (a), adding_gain);

		g *= gf;
	}

	current.g = g;
}

/* //////////////////////////////////////////////////////////////////////// */

PortInfo
PreampIV::port_info [] = 
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
Descriptor<PreampIV>::setup()
{
	UniqueID = 1777;
	Label = "PreampIV";
	Properties = HARD_RT;

	Name = CAPS "PreampIV - Tube preamp emulation + tone controls";
	Maker = "Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2002-7";

	/* fill port info and vtable */
	autogen();
}

