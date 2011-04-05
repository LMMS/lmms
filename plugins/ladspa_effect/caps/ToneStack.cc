/*
	ToneStack.cc
	
	Copyright 2006-7
		David Yeh <dtyeh@ccrma.stanford.edu> 
		Tim Goetze <tim@quitte.de> (cosmetics)

	Tone Stack emulation.
*
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

#include "ToneStack.h"
#include "Descriptor.h"

#include "dsp/tonestack/ks_tab.h"
#include "dsp/tonestack/vs_tab.h"

DSP::TSParameters 
DSP::ToneStack::presets[] = {
	/* for convenience, temporarily define k and MOhms as well as nF and pF */
	#define k * 1000
	#define M * 1000000
	#define nF * 1e-9
	#define pF * 1e-12
	/* parameter order is R1 - R4, C1 - C3 */
	/* { 250000, 1000000, 25000, 56000, 0.25e-9, 20e-9, 20e-9 }, DY */
	/* Fender */
	{250 k, 1 M, 25 k, 56 k, 250 pF, 20 nF, 20 nF}, /* 59 Bassman 5F6-A */
	{250 k, 250 k, 10 k, 100 k, 120 pF, 100 nF, 47 nF}, /* 69 Twin Reverb AA270 */
	{250 k, 250 k, 4.8 k, 100 k, 250 pF, 100 nF, 47 nF}, /* 64 Princeton AA1164 */
	/* Marshall */
	{220 k, 1 M, 22 k, 33 k, 470 pF, 22 nF, 22 nF}, /* 59/81 JCM-800 Lead 100 2203 */
	/* R4 is a 10 k fixed + 100 k pot in series actually */
	{250 k, 1 M, 25 k, 56 k, 500 pF, 22 nF, 22 nF}, /* 81 2000 Lead */

	#if 0
	{220 k, 1 M, 22 k, 33 k, 470 pF, 22 nF, 22 nF}, /* 90 JCM-900 Master 2100 (same as JCM-800) */
	{250 k, 1 M, 25 k, 33 k, 500 pF, 22 nF, 22 nF}, /* 67 Major Lead 200 */
	{250 k, 250 k, 25 k, 56 k, 250 pF, 47 nF, 47 nF}, /* undated M2199 30W solid state */
	#endif
	/* Vox -- R3 is fixed (circuit differs anyway) */
	{1 M, 1 M, 10 k, 100 k, 50 pF, 22 nF, 22 nF}, /* 59/86 AC-30 */
	#undef k
	#undef M
	#undef nF
	#undef pF
};

int DSP::ToneStack::n_presets = TS_N_PRESETS;

void
ToneStack::activate()
{ 
	tonestack.activate (ports + 2); 
}

template <sample_func_t F>
void
ToneStack::one_cycle (int frames)
{
	sample_t * s = ports[0];
	tonestack.start_cycle (ports + 1);
	sample_t * d = ports[5];

	for (int i = 0; i < frames; ++i) 
	{
		register sample_t a = s[i];
		a = tonestack.process (a + normal);
		F (d, i, a, adding_gain);
	}
}


PortInfo
ToneStack::port_info [] = 
{
	{
		"in",
		INPUT | AUDIO,
		{BOUNDED, -1, 1}
	}, {
		"model",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0 | INTEGER, 0, TS_N_PRESETS - 1} 
	}, {
		"bass",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_MID, 0, 1}
	}, {
		"mid",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_MID, 0, 1}
	}, {
		"treble",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_MID, 0, 1}
	}, {
		"out",
		OUTPUT | AUDIO,
		{0}
	}
};

template <> void
Descriptor<ToneStack>::setup()
{
	UniqueID = 2589;
	Label = "ToneStack";
	Properties = HARD_RT;

	Name = CAPS "ToneStack - Tone stack emulation";
	Maker = "David Yeh <dtyeh@ccrma.stanford.edu>";
	Copyright = "GPL, 2006-7";

	/* fill port info and vtable */
	autogen();
}

/* //////////////////////////////////////////////////////////////////////// */

template <sample_func_t F>
void
ToneStackLT::one_cycle (int frames)
{
	sample_t * s = ports[0];
	tonestack.updatecoefs (ports + 1);
	sample_t * d = ports[4];

	for (int i = 0; i < frames; ++i) 
	{
		register sample_t a = s[i];
		a = tonestack.process (a + normal);
		F (d, i, a, adding_gain);
	}
}

PortInfo
ToneStackLT::port_info [] = 
{
	{
		"in",
		INPUT | AUDIO,
		{BOUNDED, -1, 1}
	}, {
		"bass",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_MID, 0, 1}
	}, {
		"mid",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_MID, 0, 1}
	}, {
		"treble",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_MID, 0, 1}
	}, {
		"out",
		OUTPUT | AUDIO,
		{0}
	}
};

template <> void
Descriptor<ToneStackLT>::setup()
{
	UniqueID = 2590;
	Label = "ToneStackLT";
	Properties = HARD_RT;

	Name = CAPS "ToneStackLT - Tone stack emulation, lattice filter 44.1";
	Maker = "David Yeh <dtyeh@ccrma.stanford.edu>";
	Copyright = "GPL, 2006-7";

	/* fill port info and vtable */
	autogen();
}

