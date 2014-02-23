/*
	Chorus.h
	
	Copyright 2004-5 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	mono and stereo chorus/flanger units, traditional designs and some
	differentiated a bit further.

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

#ifndef _CHORUS_H_
#define _CHORUS_H_

#include "dsp/Sine.h"
#include "dsp/Roessler.h"
#include "dsp/Lorenz.h"
#include "dsp/Delay.h"
#include "dsp/OnePole.h"
#include "dsp/BiQuad.h"
#include "dsp/RBJ.h"

class ChorusStub
: public Plugin
{
	public:
		sample_t time, width, rate;
};

class ChorusI
: public ChorusStub
{
	public:
		DSP::Sine lfo;
		DSP::Delay delay;
		DSP::DelayTapA tap;

		template <sample_func_t>
			void one_cycle (int frames);
	
	public:
		static PortInfo port_info [];

		void init()
			{
				rate = .15;
				delay.init ((int) (.040 * fs));
			}

		void activate()
			{
				time = 0;
				width = 0;
				
				rate = *ports[3];
				
				delay.reset();
				tap.reset();

				lfo.set_f (rate, fs, 0);
			}

		void run (int n)
			{
				one_cycle<store_func> (n);
			}
		
		void run_adding (int n)
			{
				one_cycle<adding_func> (n);
			}
};

class StereoChorusI
: public ChorusStub
{
	public:
		sample_t rate;
		sample_t phase;

		DSP::Delay delay;

		struct {
			DSP::Sine lfo;
			DSP::DelayTapA tap;
		} left, right;

		template <sample_func_t>
			void one_cycle (int frames);
	
	public:
		static PortInfo port_info [];

		void init()
			{
				rate = .15;
				phase = .5; /* pi */

				delay.init ((int) (.040 * fs));
			}

		void activate()
			{
				time = 0;
				width = 0;

				delay.reset();

				left.tap.reset();
				right.tap.reset();

				left.lfo.set_f (rate, fs, 0);
				right.lfo.set_f (rate, fs, phase * M_PI);
			}

		void run (int n)
			{
				one_cycle<store_func> (n);
			}
		
		void run_adding (int n)
			{
				one_cycle<adding_func> (n);
			}
};

/* //////////////////////////////////////////////////////////////////////// */

#define FRACTAL_RATE 0.02

/* fractally modulated Chorus units */

class FracTap 
{
	public:
		DSP::Lorenz f1;
		DSP::Roessler f2;
		DSP::OnePoleLP lp;
		
		void init (double fs)
			{	
				lp.set_f (30. / fs); 
				f1.init (.001, frandom());
				f2.init (.001, frandom());
			}
		
		void set_rate (sample_t r)
			{
				f1.set_rate (r * FRACTAL_RATE);
				f2.set_rate (3.3 * r * FRACTAL_RATE);
			}

		/* t = time, w = width, should inline nicely */
		sample_t get (DSP::Delay & d, double t, double w)
			{
				double m = lp.process (f1.get() + .3 * f2.get());
				return d.get_cubic (t + w * m);
			}
};

class ChorusII
: public ChorusStub
{
	public:
		enum {
			Taps = 1
		};

		FracTap taps[Taps];
		DSP::BiQuad filter;
		DSP::Delay delay;

		template <sample_func_t>
			void one_cycle (int frames);
	
		void set_rate (sample_t r)
			{
				rate = r;
				for (int i = 0; i < Taps; ++i)
				{
					taps[i].set_rate (rate * (i * FRACTAL_RATE) / Taps);
					// fprintf (stderr, "[%d] %.3f\n", i, (rate * (i * FRACTAL_RATE) / Taps));
				}
			}

	public:
		static PortInfo port_info [];

		void init()
			{
				delay.init ((int) (.040 * fs));
				for (int i = 0; i < Taps; ++i)
					taps[i].init (fs);
				DSP::RBJ::HiShelve (1000. / fs, 1., 6, filter.a, filter.b);
			}

		void activate()
			{
				time = 0;
				width = 0;
				
				set_rate (*ports[3]);
				
				delay.reset();
				filter.reset();
			}

		void run (int n)
			{
				one_cycle<store_func> (n);
			}
		
		void run_adding (int n)
			{
				one_cycle<adding_func> (n);
			}
};

class StereoChorusII
: public ChorusStub
{
	public:
		sample_t rate;
		sample_t phase;

		DSP::Delay delay;

		struct {
			DSP::Roessler fractal;
			DSP::OnePoleLP lfo_lp;
			DSP::DelayTapA tap;
		} left, right;

		template <sample_func_t>
		void one_cycle (int frames);
	
		void set_rate (sample_t r)
			{
				rate = r;
				left.fractal.set_rate (rate * FRACTAL_RATE);
				right.fractal.set_rate (rate * FRACTAL_RATE);
				left.lfo_lp.set_f (3. / fs);
				right.lfo_lp.set_f (3. / fs);
			}

	public:
		static PortInfo port_info [];
		sample_t adding_gain;

		void init()
			{
				phase = .5; /* pi */

				delay.init ((int) (.040 * fs));

				left.fractal.init (.001, frandom());
				right.fractal.init (.001, frandom());
			}

		void activate()
			{
				time = 0;
				width = 0;

				delay.reset();

				left.tap.reset();
				right.tap.reset();

				set_rate (*ports[3]);
			}

		void run (int n)
			{
				one_cycle<store_func> (n);
			}
		
		void run_adding (int n)
			{
				one_cycle<adding_func> (n);
			}
};

#endif /* _CHORUS_H_ */
