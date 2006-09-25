/*
	dsp/SVF.h
	
	Copyright 2002-4 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	ladder filter in Chamberlin topology. supports largely independent 
	f and Q adjustments and sweeps.

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
/*
  inspired by this music-dsp entry:

	State Variable Filter (Double Sampled, Stable)
	Type : 2 Pole Low, High, Band, Notch and Peaking
	References :Posted by Andrew Simper

	Notes :
	Thanks to Laurent de Soras for the stability limit
	and Steffan Diedrichsen for the correct notch output.

	Code :
	input  = input buffer;
	output = output buffer;
	fs     = sampling frequency;
	fc     = cutoff frequency normally something like:
					 440.0*pow(2.0, (midi_note - 69.0)/12.0);
	res    = resonance 0 to 1;
	drive  = internal distortion 0 to 0.1
	freq   = MIN(0.25, 2.0*sin(PI*fc/(fs*2)));  // the fs*2 is because it's double sampled
	damp   = MIN(2.0*(1.0 - pow(res, 0.25)), MIN(2.0, 2.0/freq - freq*0.5));
	notch  = notch output
	low    = low pass output
	high   = high pass output
	band   = band pass output
	peak   = peaking output = low - high
	-- 
	double sampled svf loop:
	for (i=0; i<numSamples; i++)
	{
		in    = input[i];
		notch = in - damp*band;
		low   = low + freq*band;
		high  = notch - low;
		band  = freq*high + band - drive*band*band*band;
		out   = 0.5*(notch or low or high or band or peak);
		notch = in - damp*band;
		low   = low + freq*band;
		high  = notch - low;
		band  = freq*high + band - drive*band*band*band;
		out  += 0.5*(same out as above);
		output[i] = out;
	}
*/

#ifndef _DSP_SVF_H_
#define _DSP_SVF_H_

namespace DSP {

template <int OVERSAMPLE>
class SVF
{
	protected:
		/* loop parameters */
		d_sample f, q, qnorm;
		
		/* outputs (peak and notch left out) */
		d_sample lo, band, hi;
		d_sample * out;

	public:
		/* the type of filtering to do. */
		enum {
			Low = 0,
			Band = 1,
			High = 2
		};

		SVF()
			{
				set_out (Low);
				set_f_Q (.1, .1);
			}
		
		void reset()
			{
				hi = band = lo = 0;
			}

		void set_f_Q (double fc, double Q)
			{
				/* this is a very tight limit */
				f = min (.25, 2 * sin (M_PI * fc / OVERSAMPLE));

				q = 2 * cos (pow (Q, .1) * M_PI * .5);
				q = min (q, min (2., 2 / f - f * .5));
				qnorm = sqrt (fabs (q) / 2. + .001);
			}

		void set_out (int o)
			{
				if (o == Low)
					out = &lo;
				else if (o == Band)
					out = &band;
				else
					out = &hi;
			}

		void one_cycle (d_sample * s, int frames)
			{
				for (int i = 0; i < frames; ++i)
					s[i] = process (s[i]);
			}

		d_sample process (d_sample x)
			{
				x = qnorm * x;

				for (int pass = 0; pass < OVERSAMPLE; ++pass)
				{
					hi = x - lo - q * band;
					band += f * hi;
					lo += f * band;

					/* zero-padding, not 0th order holding. */
					x = 0;
				}

				/* peak and notch outputs don't belong in the loop, put them
				 * here (best in a template) if needed. */

				return *out;
			}
};

template <int STACKED, int OVERSAMPLE>
class StackedSVF
{
	public:
		SVF<OVERSAMPLE> svf [STACKED];

		void reset()
			{
				for (int i = 0; i < STACKED; ++i)
					svf[i].reset();
			}

		void set_out (int out)
			{
				for (int i = 0; i < STACKED; ++i)
					svf[i].set_out (out);
			}

		void set_f_Q (double f, double Q)
			{
				for (int i = 0; i < STACKED; ++i)
					svf[i].set_f_Q (f, Q);
			}

		d_sample process (d_sample x)
			{
				for (int i = 0; i < STACKED; ++i)
					x = svf[i].process (x);

				return x;
			}
};

} /* namespace DSP */

#endif /* _DSP_SVF_H_ */
