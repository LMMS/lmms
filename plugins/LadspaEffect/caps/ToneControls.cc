/*
	ToneControls.cc
	
	Copyright 2004-7 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	4-way Eq for amplifier emulation plugins
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

PreampBand 
ToneControls::bands[] = 
{
	/*  f,    Q,    g */
	{  80, 1.20, 1.61},
	{ 300, 1.10, 1.10},
	{1200, 1.14, 1.07},
	{4800,  .80, 1.06}
};

void
ToneControls::init (double fs)
{
	for (int i = 0; i < 4; ++i)
		eq.init_band (i, 2 * bands[i].center * M_PI / fs, bands[i].Q);
}

double 
ToneControls::get_band_gain (int i, double g)
{
	return bands[i].adjust * DSP::db2lin (g);
}

void
ToneControls::set_band_gain (int i, float g)
{
	/* sorry, _ != . but hardly readable -- the difference is between local
	 * buffered value and actual Eq band gain */
	eq_gain[i] = g;
	eq.gain[i] = get_band_gain (i, g);
}

void
ToneControls::activate (sample_t ** ports)
{
	for (int i = 0; i < 4; ++i)
		set_band_gain (i, *ports[i]);

	eq.reset();
}


