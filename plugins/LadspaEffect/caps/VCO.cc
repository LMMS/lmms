/*
	VCO.cc
	
	Copyright 2004-7 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	an oversampled triangle/saw/square oscillator, and a combination of two
	such oscillators with hard sync.

	TODO: optimize for phase clamping like this:
			phi -= floor (phi);
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

#include "VCO.h"
#include "Descriptor.h"

void
VCOs::init()
{
	/* going a fair bit lower than nominal with fc because the filter
	 * rolloff is not as steep as we might like it to be. */
	double f = .5 * M_PI / OVERSAMPLE;

	/* construct the downsampler filter kernel */
	DSP::sinc (f, down.c, FIR_SIZE);
	DSP::kaiser<DSP::apply_window> (down.c, FIR_SIZE, 6.4);

	/* normalize downsampler filter gain */
	double s = 0;
	for (int i = 0; i < down.n; ++i)
		s += down.c[i];
	
	/* scale downsampler kernel */
	s = 1 / s;
	for (int i = 0; i < down.n; ++i)
		down.c[i] *= s;
}

template <sample_func_t F>
void
VCOs::one_cycle (int frames)
{
	vco.set_f (getport(0), OVERSAMPLE * fs);
	vco.set_saw_square (getport(1), getport(2));

	double g = (gain == *ports[3]) ? 
		1 : pow (getport(3) / gain, 1. / (double) frames);

	sample_t * d = ports[4];

	for (int i = 0; i < frames; ++i)
	{
		F (d, i, gain * down.process (vco.get()), adding_gain);

		for (int o = 1; o < OVERSAMPLE; ++o)
			down.store (vco.get());

		gain *= g;
	}

	gain = getport(3);
}

/* //////////////////////////////////////////////////////////////////////// */

PortInfo
VCOs::port_info [] =
{
	{
		"f",
		INPUT | CONTROL,
		{BOUNDED | LOG | DEFAULT_100, 1, 5751}
	}, {
		"tri .. saw",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, 0, 1}
	}, {
		"~ .. square",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, 0, 1}
	}, {
		"volume",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_MID, MIN_GAIN, 1}
	}, {
		"out",
		OUTPUT | AUDIO,
		{0}
	}
};

template <> void
Descriptor<VCOs>::setup()
{
	UniqueID = 1783;
	Label = "VCOs";
	Properties = HARD_RT;

	Name = CAPS "VCOs - Virtual 'analogue' oscillator";
	Maker = "Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2004-7";

	/* fill port info and vtable */
	autogen();
}

/* //////////////////////////////////////////////////////////////////////// */

void
VCOd::init()
{
	/* going a fair bit lower than nominal with fc because the filter
	 * rolloff is not as steep as we might like it to be. */
	double f = .5 * M_PI / OVERSAMPLE;

	/* construct the downsampler filter kernel */
	DSP::sinc (f, down.c, FIR_SIZE);
	DSP::kaiser<DSP::apply_window> (down.c, FIR_SIZE, 6.4);

	/* normalize downsampler filter gain */
	double s = 0;
	for (int i = 0; i < down.n; ++i)
		s += down.c[i];
	
	/* scale downsampler kernel */
	s = 1 / s;
	for (int i = 0; i < down.n; ++i)
		down.c[i] *= s;
}

template <sample_func_t F>
void
VCOd::one_cycle (int frames)
{
	vco.set_f (getport(0), OVERSAMPLE * fs, getport(5));

	vco.vco[0].set_saw_square (getport(1), getport(2));
	vco.vco[1].set_saw_square (getport(3), getport(4));

	vco.set_sync (getport(6));
	vco.set_blend (getport(7));

	double g = (gain == *ports[8]) ? 
		1 : pow (getport(8) / gain, 1. / (double) frames);

	sample_t * d = ports[9];

	for (int i = 0; i < frames; ++i)
	{
		F (d, i, gain * down.process (vco.get()), adding_gain);

		for (int o = 1; o < OVERSAMPLE; ++o)
			down.store (vco.get());

		gain *= g;
	}

	gain = getport(8);
}

/* //////////////////////////////////////////////////////////////////////// */

PortInfo
VCOd::port_info [] =
{
	{
		"f",
		INPUT | CONTROL,
		{BOUNDED | LOG | DEFAULT_100, 1, 5751}
	}, {
		"1: tri .. saw",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, 0, 1}
	}, {
		"1: ~ .. square",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, 0, 1}
	}, {
		"2: tri .. saw",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, 0, 1}
	}, {
		"2: ~ .. square",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, 0, 1}
	}, {
		"2: tune",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, -12, 12}
	}, {
		"sync",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, 0, 1}
	}, {
		"blend",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_HIGH, -1, 1}
	}, {
		"volume",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_MID, MIN_GAIN, 1}
	}, {
		"out",
		OUTPUT | AUDIO,
		{0}
	}
};

template <> void
Descriptor<VCOd>::setup()
{
	UniqueID = 1784;
	Label = "VCOd";
	Properties = HARD_RT;

	Name = CAPS "VCOd - Double VCO with detune and hard sync options";
	Maker = "Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2004-7";

	/* fill port info and vtable */
	autogen();
}

