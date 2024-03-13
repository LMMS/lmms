/*
	Chorus.cc
	
	Copyright 2004-7 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	mono and mono-to-stereo chorus units.
	
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

#include "Chorus.h"
#include "Descriptor.h"

template <sample_func_t F>
void
ChorusI::one_cycle (int frames)
{
	sample_t * s = ports[0];

	double one_over_n = 1 / (double) frames;
	double ms = .001 * fs;

	double t = time;
	time = getport(1) * ms;
	double dt = (time - t) * one_over_n;

	double w = width;
	width = getport(2) * ms;
	/* clamp, or we need future samples from the delay line */
	if (width >= t - 3) width = t - 3;
	double dw = (width - w) * one_over_n;

	if (rate != *ports[3]) 
		lfo.set_f (max (rate = getport(3), .000001), fs, lfo.get_phase());
			
	double blend = getport(4);
	double ff = getport(5);
	double fb = getport(6);

	sample_t * d = ports[7];

	DSP::FPTruncateMode truncate;

	for (int i = 0; i < frames; ++i)
	{
		sample_t x = s[i];

		/* truncate the feedback tap to integer, better quality for less
		 * cycles (just a bit of zipper when changing 't', but it does sound
		 * interesting) */
		int ti;
		fistp (t, ti);
		x -= fb * delay[ti];

		delay.put (x + normal);

#		if 0
		/* allpass delay sounds a little cleaner for a chorus
		 * but sucks big time when flanging. */
		x = blend * x + ff * tap.get (delay, t + w * lfo.get());
#		elif 0
		/* linear interpolation */
		x = blend * x + ff * delay.get_at (t + w * lfo.get());
#		else
		/* cubic interpolation */
		x = blend * x + ff * delay.get_cubic (t + w * lfo.get());
#		endif

		F (d, i, x, adding_gain);

		t += dt;
		w += dw;
	}
}

/* //////////////////////////////////////////////////////////////////////// */

PortInfo
ChorusI::port_info [] =
{
	{
		"in",
		INPUT | AUDIO,
		{BOUNDED, -1, 1}
	}, {
		"t (ms)",
		INPUT | CONTROL,
		{BOUNDED | LOG | DEFAULT_LOW, 2.5, 40}
	}, {
		"width (ms)",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_1, .5, 10}
	}, {
		"rate (Hz)",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_LOW, 0, 5}
	}, {
		"blend",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_1, 0, 1}
	}, {
		"feedforward",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_LOW, 0, 1}
	}, {
		"feedback",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, 0, 1}
	}, {
		"out",
		OUTPUT | AUDIO,
		{0}
	}
};

template <> void
Descriptor<ChorusI>::setup()
{
	UniqueID = 1767;
	Label = "ChorusI";
	Properties = HARD_RT;

	Name = CAPS "ChorusI - Mono chorus/flanger";
	Maker = "Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2004-7";

	/* fill port info and vtable */
	autogen();
}

/* //////////////////////////////////////////////////////////////////////// */

template <sample_func_t F>
void
StereoChorusI::one_cycle (int frames)
{
	sample_t * s = ports[0];

	double one_over_n = 1 / (double) frames;
	double ms = .001 * fs;

	double t = time;
	time = getport(1) * ms;
	double dt = (time - t) * one_over_n;

	double w = width;
	width = getport(2) * ms;
	/* clamp, or we need future samples from the delay line */
	if (width >= t - 1) width = t - 1;
	double dw = (width - w) * one_over_n;

	if (rate != *ports[3] && phase != *ports[4]) 
	{
		rate = getport(3);
		phase = getport(4);
		double phi = left.lfo.get_phase();
		left.lfo.set_f (max (rate, .000001), fs, phi);
		right.lfo.set_f (max (rate, .000001), fs, phi + phase * M_PI);
	}

	double blend = getport(5);
	double ff = getport(6);
	double fb = getport(7);

	sample_t * dl = ports[8];
	sample_t * dr = ports[9];

	/* to go sure (on i386) that the fistp instruction does the right thing 
	 * when looking up fractional sample indices */
	DSP::FPTruncateMode truncate;

	for (int i = 0; i < frames; ++i)
	{
		sample_t x = s[i];

		/* truncate the feedback tap to integer, better quality for less
		 * cycles (just a bit of zipper when changing 't', but it does sound
		 * interesting) */
		int ti;
		fistp (t, ti);
		x -= fb * delay[ti];

		delay.put (x + normal);

		sample_t l = blend * x + ff * delay.get_cubic (t + w * left.lfo.get());
		sample_t r = blend * x + ff * delay.get_cubic (t + w * right.lfo.get());

		F (dl, i, l, adding_gain);
		F (dr, i, r, adding_gain);

		t += dt;
		w += dw;
	}
}

/* //////////////////////////////////////////////////////////////////////// */

PortInfo
StereoChorusI::port_info [] =
{
	{
		"in",
		INPUT | AUDIO,
		{BOUNDED, -1, 1}
	}, {
		"t (ms)",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_MIN, 2.5, 40}
	}, {
		"width (ms)",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_1, .5, 10}
	}, {
		"rate (Hz)",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_LOW, 0, 5}
	}, {
		"phase",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_MAX, 0, 1}
	}, {
		"blend",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_1, 0, 1}
	}, {
		"feedforward",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_LOW, 0, 1}
	}, {
		"feedback",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, 0, 1}
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
Descriptor<StereoChorusI>::setup()
{
	UniqueID = 1768;
	Label = "StereoChorusI";
	Properties = HARD_RT;

	Name = CAPS "StereoChorusI - Stereo chorus/flanger";
	Maker = "Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2004-7";

	/* fill port info and vtable */
	autogen();
}

/* //////////////////////////////////////////////////////////////////////// */

template <sample_func_t F>
void
ChorusII::one_cycle (int frames)
{
	sample_t * s = ports[0];

	double one_over_n = 1 / (double) frames;
	double ms = .001 * fs;

	double t = time;
	time = getport(1) * ms;
	double dt = (time - t) * one_over_n;

	double w = width;
	width = getport(2) * ms;
	/* clamp, or we need future samples from the delay line */
	if (width >= t - 3) width = t - 3;
	double dw = (width - w) * one_over_n;

	if (rate != *ports[3])
		set_rate (*ports[3]);
			
	double blend = getport(4);
	double ff = getport(5);
	double fb = getport(6);

	sample_t * d = ports[7];

	DSP::FPTruncateMode truncate;

	for (int i = 0; i < frames; ++i)
	{
		sample_t x = s[i];

		x -= fb * delay.get_cubic (t);

		delay.put (filter.process (x + normal));

		double a = 0;
		for (int j = 0; j < Taps; ++j)
			a += taps[j].get (delay, t, w);

		x = blend * x + ff * a;

		F (d, i, x, adding_gain);

		t += dt;
		w += dw;
	}
}

/* //////////////////////////////////////////////////////////////////////// */

PortInfo
ChorusII::port_info [] =
{
	{
		"in",
		INPUT | AUDIO,
		{BOUNDED, -1, 1}
	}, {
		"t (ms)",
		INPUT | CONTROL,
		{BOUNDED | LOG | DEFAULT_LOW, 2.5, 40}
	}, {
		"width (ms)",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_1, .5, 10}
	}, {
		"rate",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_LOW, 0, 1}
	}, {
		"blend",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_1, 0, 1}
	}, {
		"feedforward",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_LOW, 0, 1}
	}, {
		"feedback",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, 0, 1}
	}, {
		"out",
		OUTPUT | AUDIO,
		{0}
	}
};

template <> void
Descriptor<ChorusII>::setup()
{
	UniqueID = 2583;
	Label = "ChorusII";
	Properties = HARD_RT;

	Name = CAPS "ChorusII - Mono chorus/flanger modulated by a fractal";
	Maker = "Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2004-7";

	/* fill port info and vtable */
	autogen();
}

/* //////////////////////////////////////////////////////////////////////// */

template <sample_func_t F>
void
StereoChorusII::one_cycle (int frames)
{
	sample_t * s = ports[0];

	double one_over_n = 1 / (double) frames;
	double ms = .001 * fs;

	double t = time;
	time = getport(1) * ms;
	double dt = (time - t) * one_over_n;

	double w = width;
	width = getport(2) * ms;
	/* clamp, or we need future samples from the delay line */
	if (width >= t - 1) width = t - 1;
	double dw = (width - w) * one_over_n;

	set_rate (*ports[3]);

	double blend = getport(4);
	double ff = getport(5);
	double fb = getport(6);

	sample_t * dl = ports[7];
	sample_t * dr = ports[8];

	/* to go sure (on i386) that the fistp instruction does the right thing 
	 * when looking up fractional sample indices */
	DSP::FPTruncateMode truncate;

	for (int i = 0; i < frames; ++i)
	{
		sample_t x = s[i];

		/* truncate the feedback tap to integer, better quality for less
		 * cycles (just a bit of zipper when changing 't', but it does sound
		 * interesting) */
		int ti;
		fistp (t, ti);
		x -= fb * delay[ti];

		delay.put (x + normal);

		double m;
		m = left.lfo_lp.process (left.fractal.get());
		sample_t l = blend * x + ff * delay.get_cubic (t + w * m);
		m = right.lfo_lp.process (right.fractal.get());
		sample_t r = blend * x + ff * delay.get_cubic (t + w * m);

		F (dl, i, l, adding_gain);
		F (dr, i, r, adding_gain);

		t += dt;
		w += dw;
	}
}

/* //////////////////////////////////////////////////////////////////////// */

PortInfo
StereoChorusII::port_info [] =
{
	{
		"in",
		INPUT | AUDIO,
		{BOUNDED, -1, 1}
	}, {
		"t (ms)",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_LOW, 2.5, 40}
	}, {
		"width (ms)",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_LOW, .5, 10}
	}, {
		"rate",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_LOW, 0, 1}
	}, {
		"blend",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_1, 0, 1}
	}, {
		"feedforward",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_MID, 0, 1}
	}, {
		"feedback",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, 0, 1}
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
Descriptor<StereoChorusII>::setup()
{
	UniqueID = 2584;
	Label = "StereoChorusII";
	Properties = HARD_RT;

	Name = CAPS "StereoChorusII - Stereo chorus/flanger modulated by a fractal";
	Maker = "Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2004-7";

	/* fill port info and vtable */
	autogen();
}


