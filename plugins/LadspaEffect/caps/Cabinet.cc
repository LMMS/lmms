/*
	Cabinet.cc
	
	Copyright 2002-7 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	CabinetI - 16th order IIR filters modeled after various impulse responses 
	from Steve Harris' 'imp' plugin. Limited to 44.1 kHz sample rate.

	CabinetII - 32nd order IIR filters modeled after the same impulse responses
	using a different algorithm. Versions for 44.1 / 48 / 88.2 / 96 kHz sample
	rates, switched at runtime.
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

#include "Cabinet.h"
#include "Descriptor.h"

Model16
CabinetI::models [] = 
{
	{
		1, /* identity */
		{1},
		{0},
		1,
	}, {
		16, /* unmatched, off-axis */
		{0.44334744339382504, 0.49764265352620912, 0.19936863766114088, 0.0038388115609433826, -0.072080744430548876, -0.092589757815768933, -0.023760971045285254, 0.058929802988203807, 0.11073296313735595, 0.14389046518738619, 0.052981055774577353, -0.11817321919764193, -0.22957467728465422, -0.26301284181138151, -0.25586853638823448, -0.10768194462289554},
		{0.0, 0.71138736215182674, 0.19571088506350195, -0.086897678126924227, -0.18344238266155902, -0.13338660100611671, -0.017424587504494098, 0.062014975470330351, 0.077979014877680469, 0.039134631327482176, -0.046646061538403727, -0.040653480351542266, 0.068462230153713749, 0.14313162261479676, 0.058844200671679531, -0.17878548463526983},
		1 / 4.4,
	}, {
		16, /* unmatched */
		{0.58952373363852417, 0.43625942509584309, 0.235412203403113, 0.048252951521505258, -0.1002076253350956, -0.14467958356216026, -0.074304873528329402, 0.060945557247412331, 0.15959845551788171, 0.20970924189636411, 0.15361368007229703, 0.025339061869183478, -0.11824836967489599, -0.19384403813690479, -0.19295873693806628, -0.058904754435169508},
		{0.0, 0.6596380198994729, 0.20825670531468143, -0.10508784276271754, -0.24698905246649294, -0.21519418569376192, -0.10057489587096909, -0.006175810942624007, -0.0019249936836196989, -0.045827268144865707, -0.10184708094147235, -0.09177820187618313, -0.017740067693127175, 0.058828573780348989, 0.010559807576350673, -0.19412688465216324},
		1 / 5.7,
	}, {
		16, /* superchamp */
		{0.5335550129666069, 0.61768496153556829, 0.2080126894040209, -0.0067283013134491337, -0.10433152421155355, -0.16159936513841233, -0.10593018443866807, -0.091854930675998661, 0.0023550324496513643, 0.14327516190088724, 0.14688292534697539, 0.089539872443625601, -0.053854769352683005, -0.15693904535289377, -0.083502230074838577, 0.0090113128614708465},
		{0.0, 0.67745858591653696, 0.044802106922746734, -0.1932642251200547, -0.18922360327572652, -0.052980570047914469, 0.020446992577988904, -0.028951451474818618, -0.085212072485126716, -0.12241212382510525, -0.089259371449674357, 0.010469056928395087, 0.049019357277555603, 0.037212453438250505, -0.046374612934843573, -0.045133341919937828},
		1 / 4.0,
	}, {
		16, /* fender-vibrolux-68 */
		{0.68447985102464837, 0.61538710771230021, -0.28804707230137599, -0.23656583372846893, 0.083100874242250655, -0.027792816938813913, 0.055558334440593965, 0.044458161718059774, -0.25467542393376252, -0.33660492613760157, -0.024663941486403884, 0.1172751972420942, -0.021832135450802759, 0.21440383631745469, 0.20828506390107443, -0.10289957687018361},
		{0.0, 0.50968169360260451, -0.48159141882733714, -0.10856607456906017, -0.026802006374108955, -0.24967309940249552, -0.21422424792787859, 0.019271890369619571, 0.08065394481240884, -0.061665636719946543, 0.031613782215493547, 0.069574436103950893, -0.07180222507768147, -0.14447996563879059, 0.012044815820150268, -0.0073237976200291044},
		1 / 2.0,
	}, {
		16, /* marshall */
		{0.38455665328113242, 0.50304089890302783, 0.33653970418909934, 0.085604896315814846, -0.070239383452479598, -0.10479060878654689, -0.060150883776583335, 0.030121882977878822, 0.12441775056532201, 0.18287316824579156, 0.17035705141865287, 0.10315414401519916, 0.036357097566567576, 0.024474446155666255, 0.042359967009557103, 0.059946316626725109},
		{0.0, 0.68167571829534335, 0.16877527811114035, -0.17427551663276897, -0.25780056810728452, -0.16065744581310681, -0.032007062964857856, 0.033882840656718101, -0.0038880045892747792, -0.084876415098991506, -0.13865107122780057, -0.10073571899064113, -0.013199668806255366, 0.038170305284592504, -0.026492576852036546, -0.12667775510054707},
		1 / 4.2,
	}
};

/* //////////////////////////////////////////////////////////////////////// */

void
CabinetI::init()
{
	h = 0;
	model = 0;
}

void
CabinetI::switch_model (int m)
{
	if (m < 0) m = 0;
	else if (m > 5) m = 5;

	model = m;

	n = models[m].n;
	a = models[m].a;
	b = models[m].b;

	gain = models[m].gain * DSP::db2lin (getport(2));

	memset (x, 0, sizeof (x));
	memset (y, 0, sizeof (y));
}

void
CabinetI::activate()
{
	switch_model ((int) getport(1));
	gain = models[model].gain * DSP::db2lin (getport(2));
}

template <sample_func_t F>
void
CabinetI::one_cycle (int frames)
{
	sample_t * s = ports[0];

	int m = (int) getport (1);
	if (m != model) switch_model (m);

	sample_t g = models[model].gain * DSP::db2lin (getport(2));
	double gf = pow (g / gain, 1 / (double) frames);

	sample_t * d = ports[3];

	for (int i = 0; i < frames; ++i)
	{
		register cabinet_float out = s[i] + normal;
		
		x[h] = out;
		
		out *= a[0];

		for (int j = 1, z = h - 1; j < n; --z, ++j)
		{
			z &= 15;
			out += a[j] * x[z];
			out += b[j] * y[z];
		}

		y[h] = out;

		h = (h + 1) & 15;
		
		F (d, i, gain * out, adding_gain);
		gain *= gf;
	}
}

/* //////////////////////////////////////////////////////////////////////// */

PortInfo
CabinetI::port_info [] =
{
	{
		"in",
		INPUT | AUDIO,
		{BOUNDED, -1, 1}
	}, {
		"model",
		INPUT | CONTROL,
		{BOUNDED | INTEGER | DEFAULT_1, 0, 5}
	}, {
		"gain (dB)",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, -24, 24}
	}, {
		"out",
		OUTPUT | AUDIO,
		{0}
	}
};

/* //////////////////////////////////////////////////////////////////////// */

template <> void
Descriptor<CabinetI>::setup()
{
	UniqueID = 1766;
	Label = "CabinetI";
	Properties = HARD_RT;

	Name = CAPS "CabinetI - Loudspeaker cabinet emulation";
	Maker = "Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2002-7";

	/* fill port info and vtable */
	autogen();
}

/* CabinetII ////////////////////////////////////////////////////////////// */

#include "Cabinet-Models32.h"

void
CabinetII::init()
{
	if (fs < 46000)
		models = models44100;
	else if (fs < 72000)
		models = models48000;
	else if (fs < 92000)
		models = models88200;
	else 
		models = models96000;

	h = 0;
	model = 0;
}

void
CabinetII::switch_model (int m)
{
	model = m;

	n = models[m].n;
	a = models[m].a;
	b = models[m].b;

	gain = models[m].gain * DSP::db2lin (getport(2));

	memset (x, 0, sizeof (x));
	memset (y, 0, sizeof (y));
}

void
CabinetII::activate()
{
	switch_model ((int) getport(1));
}

template <sample_func_t F>
void
CabinetII::one_cycle (int frames)
{
	sample_t * s = ports[0];

	int m = (int) getport (1);
	if (m != model) switch_model (m);

	sample_t g = models[model].gain * DSP::db2lin (getport(2));
	double gf = pow (g / gain, 1 / (double) frames);
	sample_t * d = ports[3];

	for (int i = 0; i < frames; ++i)
	{
		register cabinet_float out = s[i] + normal;
		
		x[h] = out;
		
		out *= a[0];

		for (int j = 1, z = h - 1; j < n; --z, ++j)
		{
			z &= 31;
			out += a[j] * x[z];
			out += b[j] * y[z];
		}

		y[h] = out;

		h = (h + 1) & 31;
		
		F (d, i, gain * out, adding_gain);
		gain *= gf;
	}
}

/* //////////////////////////////////////////////////////////////////////// */

PortInfo
CabinetII::port_info [] =
{
	{
		"in",
		INPUT | AUDIO,
		{BOUNDED, -1, 1}
	}, {
		"model",
		INPUT | CONTROL,
		{BOUNDED | INTEGER | DEFAULT_1, 0, 7}
	}, {
		"gain (dB)",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, -24, 24}
	}, {
		"out",
		OUTPUT | AUDIO,
		{0}
	}
};

/* //////////////////////////////////////////////////////////////////////// */

template <> void
Descriptor<CabinetII>::setup()
{
	UniqueID = 2581;
	Label = "CabinetII";
	Properties = HARD_RT;

	Name = CAPS "CabinetII - Refined loudspeaker cabinet emulation";
	Maker = "Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2002-7";

	/* fill port info and vtable */
	autogen();
}

