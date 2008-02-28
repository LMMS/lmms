/*
	Compress.cc
	
	Copyright 2004-7 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	mono compressor suitable for solo instruments. adaptation of Steve
	Harris' 'sc1' unit, with minor tweaks: table lookup for attack and
	release time to frames mapping replaced with exp. port order changed.
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

#include "Compress.h"
#include "Descriptor.h"

template <sample_func_t F>
void
Compress::one_cycle (int frames)
{
	d_sample * s = ports[0];

	d_sample range = DSP::db2lin (getport(1)); 
	d_sample ratio = (*ports[2] - 1) / getport(2);

	/* sc1 has lookup tables here, and they're only 40 % used (400 ms/1 s). 
	 * thus, sc1's attack/release controls are a bit coarse due to truncation 
	 * error. calling exp() once per cycle doesn't seem too expensive. 
	 *
	 * besides, i have a suspicion that the attack and release parameters
	 * don't work like they should. if they, as the code suggests, control
	 * an exponential envelope fade, pow (.5, n_frames) or something like
	 * that seems more appropriate. so ...
	 *
	 * TODO: check whether these parameters work like they should, try pow()
	 */
	double ga = exp (-1 / (fs * getport(3))); 
	double gr = exp (-1 / (fs * getport(4)));

	d_sample threshold = getport(5);
	d_sample knee = getport(6);

	d_sample * d = ports[7];

	d_sample knee0 = DSP::db2lin (threshold - knee);
	d_sample knee1 = DSP::db2lin (threshold + knee);

	d_sample ef_a = ga * .25;
	d_sample ef_ai = 1 - ef_a;
	
	for (int i = 0; i < frames; ++i)
	{
		sum += s[i] * s[i];

		if (amp > env)
			env = env * ga + amp * (1 - ga);
		else
			env = env * gr + amp * (1 - gr);

		if ((count++ & 3) == 3)
		{
			amp = rms.process (sum * .25);
			sum = 0;
			
			if (env < knee0)
				gain_t = 1;
			else if (env < knee1)
			{
				float x = -(threshold - knee - DSP::lin2db (env)) / knee;
				gain_t = DSP::db2lin (-knee * ratio * x * x * 0.25f);
			}
			else
				gain_t = DSP::db2lin ((threshold - DSP::lin2db (env)) * ratio);
		}

	  gain = gain * ef_a + gain_t * ef_ai;

		F (d, i, s[i] * gain * range, adding_gain);
	}
}

/* //////////////////////////////////////////////////////////////////////// */

PortInfo
Compress::port_info [] =
{
	{
		"in",
		INPUT | AUDIO,
		{BOUNDED, -1, 1}
	}, {
		"gain (dB)",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_LOW, 0, 24}
	}, {
		"ratio (1:n)",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_MIN, 1, 10}
	}, {
		"attack (s)",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_MIN, .001, 1}
	}, {
		"release (s)",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_MID, .001, 1}
	}, {
		"threshold (dB)",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, -30, 400}
	}, {
		"knee radius (dB)",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_LOW, 1, 10}
	}, {
		"out",
		OUTPUT | AUDIO,
		{0}
	}
};

template <> void
Descriptor<Compress>::setup()
{
	UniqueID = 1772;
	Label = "Compress";
	Properties = HARD_RT;

	Name = CAPS "Compress - Mono compressor";
	Maker = "Tim Goetze <tim@quitte.de>, Steve Harris <steve@plugin.org.uk>";
	Copyright = "GPL, 2004-7";

	/* fill port info and vtable */
	autogen();
}

