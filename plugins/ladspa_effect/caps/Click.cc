/*
	Click.cc
	
	Copyright 2002-7 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	Plugins playing a sound snippet in regular intervals.

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

#include "waves/click.h"
#include "waves/money.h"

#include "Click.h"
#include "Descriptor.h"

void
ClickStub::init (float * _wave, int _N)
{
	wave = _wave;
	N = _N;
	bpm = -1;
}

template <sample_func_t F>
void
ClickStub::one_cycle (int frames)
{
	bpm = getport(0);
	sample_t gain = getport(1) * *ports[1];
	lp.set (1 - *ports[2]);
	
	sample_t * d = ports[3];

	while (frames)
	{
		if (period == 0)
		{
			period = (int) (fs * 60 / bpm);
			played = 0;
		}

		int n = min (frames, period);
				
		if (played < N)
		{
			n = min (n, N - played);

			for (int i = 0; i < n; ++i)
			{
				sample_t x = gain * wave [played + i] + normal;
				F (d, i, lp.process (x), adding_gain);
				normal = -normal;
			}

			played += n;
		}
		else 
		{
			for (int i = 0; i < n; ++i)
			{
				F (d, i, lp.process (normal), adding_gain);
				normal = -normal;
			}
		}

		period -= n;
		frames -= n;
		d += n;
	}
}

/* //////////////////////////////////////////////////////////////////////// */

PortInfo
ClickStub::port_info [] =
{
	{
		"bpm",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_LOW, 4, 244}
	}, {
		"volume",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_MID, 0, 1}
	}, {
		"damping",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_HIGH, 0, 1}
	}, {
		"out",
		OUTPUT | AUDIO,
		{0}
	}
};

/* //////////////////////////////////////////////////////////////////////// */

#define LENGTH(W) ((int) (sizeof (W) / sizeof (float)))

void
Click::init()
{
	this->ClickStub::init (click, LENGTH (click));
}

template <> void
Descriptor<Click>::setup()
{
	UniqueID = 1769;
	Label = "Click";
	Properties = HARD_RT;

	Name = CAPS "Click - Metronome";
	Maker = "Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2004-7";

	/* fill port info and vtable */
	autogen();
}

/* //////////////////////////////////////////////////////////////////////// */

PortInfo
CEO::port_info [] =
{
	{
		"mpm",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_HIGH, 4, 244}
	}, {
		"volume",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_MID, 0, 1}
	}, {
		"damping",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_MIN, 0, 1}
	}, {
		"out",
		OUTPUT | AUDIO,
		{0}
	}
};

void
CEO::init()
{ 
	this->ClickStub::init (money, LENGTH (money));
}

template <> void
Descriptor<CEO>::setup()
{
	UniqueID = 1770;
	Label = "CEO";
	Properties = HARD_RT;

	Name = CAPS "CEO - Chief Executive Oscillator";
	Maker = "Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2004-7";

	/* fill port info and vtable */
	autogen();
}

/* //////////////////////////////////////////////////////////////////////// */

float dirac [] = { 1, };

PortInfo
Dirac::port_info [] =
{
	{
		"ppm",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_MIN, 30, 60}
	}, {
		"volume",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, 0, 1}
	}, {
		"damping",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, 0, 1}
	}, {
		"out",
		OUTPUT | AUDIO,
		{0}
	}
};

void
Dirac::init()
{ 
	this->ClickStub::init (dirac, LENGTH (dirac));
}

template <> void
Descriptor<Dirac>::setup()
{
	UniqueID = 2585;
	Label = "Dirac";
	Properties = HARD_RT;

	Name = CAPS "Dirac - One-sample impulse generator";
	Maker = "Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2004-7";

	/* fill port info and vtable */
	autogen();
}


