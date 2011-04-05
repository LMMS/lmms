/*
	Amp.h
	
	Copyright 2002-9 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	Oversampled tube amplifier emulation.

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

#ifndef _AMP_H_
#define _AMP_H_

#include "dsp/util.h"
#include "dsp/OnePole.h"
#include "dsp/BiQuad.h"
#include "dsp/TwelveAX7.h"
#include "dsp/Roessler.h"

#include "dsp/FIR.h"
#include "dsp/sinc.h"
#include "dsp/windows.h"

#include "dsp/RBJ.h"
#include "dsp/Eq.h"

#include "dsp/ToneStack.h"

class AmpStub
: public Plugin
{
	public:
		DSP::TwelveAX7_3 tube;
		
		sample_t drive, i_drive;

		struct {
			/* gain (remember current setting and fade to port setting in run) */
			double g;
			/* should also do this for temperature to remove another potential
			 * source of zippering, but that would be overkill, at the cost of
			 * at least one pow() per block. */
		} current;

		/* input is hipass-filtered first */
		DSP::OnePoleHP dc_blocker;

		enum {
			OVERSAMPLE = 8,
			FIR_SIZE = 64,
		};

		/* antialias filters */
		DSP::FIRUpsampler up;
		DSP::FIR down;

		AmpStub()
			: up (FIR_SIZE, OVERSAMPLE), 
				down (FIR_SIZE, up.c)
			{ }
		
		void init (bool adjust_downsampler = false);

		inline sample_t power_transfer (sample_t a)
			{
				return i_drive * (a - drive * fabs (a) * a);
			}
};

/* /////////////////////////////////////////////////////////////////////// */

class PreampIII
: public AmpStub
{
	public:
		template <sample_func_t F, int OVERSAMPLE>
			void one_cycle (int frames);

		DSP::BiQuad filter;

	public:
		static PortInfo port_info[];

		sample_t adding_gain;

		void init();
		void activate()
			{
				current.g = 1;

				filter.reset();
				up.reset();
				down.reset();
				dc_blocker.reset();
			}

		void run (int n)
			{
				one_cycle<store_func, OVERSAMPLE> (n);
			}
		
		void run_adding (int n)
			{
				one_cycle<adding_func, OVERSAMPLE> (n);
			}
};

/* /////////////////////////////////////////////////////////////////////// */

class AmpIII
: public AmpStub
{
	public:
		template <sample_func_t F, int OVERSAMPLE>
			void one_cycle (int frames);

		DSP::BiQuad filter;

	public:
		static PortInfo port_info[];

		sample_t adding_gain;

		void init();
		void activate()
			{
				current.g = 1;

				up.reset();
				down.reset();
				dc_blocker.reset();
				filter.reset();
			}

		void run (int n)
			{
				one_cycle<store_func, OVERSAMPLE> (n);
			}
		
		void run_adding (int n)
			{
				one_cycle<adding_func, OVERSAMPLE> (n);
			}
};

/* /////////////////////////////////////////////////////////////////////// */

typedef struct 
	{float center, Q, adjust;}
PreampBand;

class ToneControls 
{
	public:
		sample_t eq_gain[4];
		DSP::Eq<4> eq;
		static PreampBand bands[4];
		
	public:
		void init (double _fs);
		void activate (sample_t **);

		inline void 
		start_cycle (sample_t ** ports, double one_over_n)
			{
				for (int i = 0; i < 4; ++i)
				{
					if (*ports[i] == eq_gain[i])
					{
						eq.gf[i] = 1;
						continue;
					}

					eq_gain[i] = *ports [i];

					double want = get_band_gain (i, eq_gain[i]);
					eq.gf[i] = pow (want / eq.gain[i], one_over_n);
				}
			}

		double get_band_gain (int i, double g);
		void set_band_gain (int i, float g);

		inline sample_t process (sample_t x)
			{
				return eq.process (x);
			}
};

/* /////////////////////////////////////////////////////////////////////// */

class PreampIV
: public PreampIII
{
	public:
		ToneControls tone;

		template <sample_func_t F, int OVERSAMPLE>
			void one_cycle (int frames);

	public:
		static PortInfo port_info[];

		sample_t adding_gain;

		void init();
		void activate();

		void run (int n)
			{
				one_cycle<store_func, OVERSAMPLE> (n);
			}
		
		void run_adding (int n)
			{
				one_cycle<adding_func, OVERSAMPLE> (n);
			}
};

/* /////////////////////////////////////////////////////////////////////// */

class AmpIV
: public AmpStub
{
	public:
		ToneControls tone;

		template <sample_func_t F, int OVERSAMPLE>
			void one_cycle (int frames);

	public:
		static PortInfo port_info[];

		sample_t adding_gain;

		void init();
		void activate()
			{
				current.g = 1;

				tone.activate (ports + 3);

				up.reset();
				down.reset();
				dc_blocker.reset();
			}

		void run (int n)
			{
				one_cycle<store_func, OVERSAMPLE> (n);
			}
		
		void run_adding (int n)
			{
				one_cycle<adding_func, OVERSAMPLE> (n);
			}
};

/* /////////////////////////////////////////////////////////////////////// */

class AmpV
: public AmpStub
{
	public:
		template <sample_func_t F, int OVERSAMPLE>
			void one_cycle (int frames);

		DSP::BiQuad filter[3];
		
		sample_t cut, tone;

		/* supply voltage sag */
		sample_t supply;
		DSP::BiQuad power_cap[2];
		
	public:
		static PortInfo port_info[];

		sample_t adding_gain;

		void init();
		void activate()
			{
				current.g = 1;

				for (int i = 0; i < 2; ++i)
					filter[i].reset(),
					power_cap[i].reset();

				up.reset();
				down.reset();
				dc_blocker.reset();

				cut = 2;
				supply = 0.;

				tone = -1; /* causes initialisation of the filter at first cycle */
			}

		void run (int n)
			{
				one_cycle<store_func, OVERSAMPLE> (n);
			}
		
		void run_adding (int n)
			{
				one_cycle<adding_func, OVERSAMPLE> (n);
			}
};

/* /////////////////////////////////////////////////////////////////////// */

class AmpVTS
: public AmpStub
{
	public:
		DSP::ToneStack tonestack;

		template <sample_func_t F, int OVERSAMPLE>
			void one_cycle (int frames);

		sample_t cut, tone;

		/* supply voltage sag */
		sample_t supply;
		DSP::BiQuad power_cap[2];
		
	public:
		static PortInfo port_info[];

		sample_t adding_gain;

		void init();
		void activate()
			{
				current.g = 1;

				for (int i = 0; i < 2; ++i)
					power_cap[i].reset();

				up.reset();
				down.reset();
				dc_blocker.reset();

				cut = 2;
				supply = 0.;
			}

		void run (int n)
			{
				one_cycle<store_func, OVERSAMPLE> (n);
			}
		
		void run_adding (int n)
			{
				one_cycle<adding_func, OVERSAMPLE> (n);
			}
};

#endif /* _AMP_H_ */
