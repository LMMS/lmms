/*
  dsp/VCO.h

	Copyright 2004 Tim Goetze <tim@quitte.de>

	oscillators for triangle/sawtooth/square waves, and a combination
	for detuning and hard sync.

	NB: these oscillators are *not* bandlimited. oversample if needed.

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


#ifndef _DSP_VCO_H_
#define _DSP_VCO_H_

namespace DSP {
	
/* variable triangle to sawtooth generator. you can use two of these to
 * generate a square, but we prefer the integrated solution below.
 */
class TriSaw
{
	public:
		/* doubles for maximum stability */
		double phase, inc;
		
		double tri, tri1, tri2;
		
	public:
		TriSaw()
			{ 
				phase = 0;
				tri = .5;
			}

		inline void set_f (double f, double fs)
			{
				set_inc (f / fs);
			}

		inline void set_inc (double i)
			{
				inc = i;
			}

		/* 0: triangle, 1: saw */
		inline void set_saw (double t)
			{
				tri = .5 + .5 * t;
				tri1 = 2. / tri;
				tri2 = 2. / (1 - tri);
			}

		/* advance and return 1 sample.
		 * many conditionals, but quicker than a solution based on fmod()
		 */
		inline float get()
			{
				phase += inc;
				
				/* the good thing is that tri is always > .5, which implies
				 * that this first conditional is true more often than not. */
				if (phase <= tri)
					return -1 + phase * tri1;
				if (phase < 1)
					return 1 - (phase - tri) * tri2;
				
				phase -= 1;
				return -1 + phase * tri1;
			}
};

/* variable triangle to sawtooth to square generator */
class TriSawSquare
{
	public:
		/* doubles for maximum stability, using floats here increases
		 * cycle need on my athlon */
		double phase, inc;
		double * sync;

		float sync_phase;

		/* using doubles here increases cycle need significantly */
		float square_i;
		float tri, tri1, tri2;
		float st1, st2;

	public:
		TriSawSquare()
			{ 
				reset();
			}

		void reset()
			{
				phase = 0;
				sync = &phase;
				sync_phase = 0;
				set_saw_square (.5, .5);
			}

		inline void set_f (double f, double fs)
			{
				set_inc (f / fs);
			}

		inline void set_inc (double i)
			{
				inc = i;
			}

		inline void set_sync (TriSawSquare & tss, float p)
			{
				sync = &tss.phase;
				sync_phase = p;
			}

		/* t = 0: tri     - 1: saw, 
		 * s = 0: tri/saw - 1: square 
		 */
		inline void set_saw_square (float t, float s)
			{
				tri = .5 + .5 * t;
				square_i = 1 - s;

				float si2 = 2 * square_i;
				float one_m_t = 1 - tri;

				tri1 = si2 / tri;
				tri2 = si2 / one_m_t;

				st1 = s * one_m_t;
				st2 = s * tri;
			}

		/* advance and return 1 sample. a pity we need so many conditionals,
		 * seeing that this is run at 352 k.
		 */
		inline float get()
			{
				phase += inc;
				
				if (phase <= tri)
					first_half:
					/* raw version:
					return (1 - square) * (-1 + phase * 2 / tri) - square * (1 - tri);
					 */
					return -square_i + phase * tri1 - st1;

				if (phase < 1)
					/* raw version:
					return (1 - square) * (1 - (phase - tri) * 2 / (1 - tri)) + square * tri;
					 */
					return square_i - (phase - tri) * tri2 + st2;
				
				phase -= 1;
				*sync = phase + sync_phase;
				goto first_half;
			}
};

class VCO2
{
	public:
		TriSawSquare vco[2];
		float blend, i_blend;

	public:
		VCO2()
			{
				set_blend (.5);
			}
		
		void reset()
			{
				set_blend (.5);
				vco[0].reset();
				vco[1].reset();
			}

		void set_f (double f, double fs, double detune)
			{
				vco[0].set_f (f, fs);
				vco[1].set_f (f * pow (2, detune / 12.), fs);
			}

		inline void set_blend (float b)
			{
				blend = b;
				i_blend = 1 - fabs (b);
			}
				
		inline void set_sync (float sync)
			{
				vco[0].set_sync (sync ? vco[1] : vco[0], sync);
			}

		inline float get()
			{
				return vco[0].get() * blend + vco[1].get() * i_blend;
			}
};

} /* namespace DSP */

#endif /* _DSP_VCO_H_ */
