/*
  dsp/RBJ.h

	Copyright 2004 Tim Goetze <tim@quitte.de>, 1998 Robert Bristow-Johnson

	biquad prototypes according to the eq cookbook. easy-to-use, nice, 
	predictable filters. thanks rbj!
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

#ifndef _DSP_RBJ_H_
#define _DSP_RBJ_H_

namespace DSP { 
namespace RBJ {

/* base class, prepares common parameters */
class RBJ
{
	public:
		double alpha, sin, cos;
		double a[3], b[3];

	public:
		RBJ (double f, double Q)
			{
				double w = 2 * M_PI * f;

				sin = ::sin (w);
				cos = ::cos (w);

				alpha = sin / (2 * Q);
			}

		/* templated so we can set double and float coefficients from the same
		 * piece of code */
		template <class T>
		void make_direct_I (T * ca, T * cb)
			{
				double a0i = 1 / a[0];
				
				ca[0] = b[0] * a0i;
				ca[1] = b[1] * a0i;
				ca[2] = b[2] * a0i;

				/* our bi-quad implementation /adds/ b[i] * y[i] so we need to 
				 * toggle the sign for the b[] coefficients. 
				 */
				cb[0] = 0;
				cb[1] = -a[1] * a0i;
				cb[2] = -a[2] * a0i;
			}
};

/* now the individual prototypes. 
 * set-up is not optimal, i.e. does a lot of operations twice for readability.
 */
class LP
: public RBJ
{
	public:
		template <class T>
		LP (double f, double Q, T * ca, T * cb)
			: RBJ (f, Q)
			{
				b[0] = (1 - cos) * .5;
				b[1] = (1 - cos);
				b[2] = (1 - cos) * .5;
				
				a[0] = 1 + alpha;
				a[1] = -2 * cos;
				a[2] = 1 - alpha;
				
				make_direct_I (ca, cb);
			}
};

class BP
: public RBJ
{
	public:
		template <class T>
		BP (double f, double Q, T * ca, T * cb)
			: RBJ (f, Q)
			{
				b[0] = Q * alpha;
				b[1] = 0;
				b[2] = -Q * alpha;
				
				a[0] = 1 + alpha;
				a[1] = -2 * cos;
				a[2] = 1 - alpha;
				
				make_direct_I (ca, cb);
			}
};

class HP
: public RBJ
{
	public:
		template <class T>
		HP (double f, double Q, T * ca, T * cb)
			: RBJ (f, Q)
			{
				b[0] =  (1 + cos) * .5;
				b[1] = -(1 + cos);
				b[2] =  (1 + cos) * .5;
				
				a[0] = 1 + alpha;
				a[1] = -2 * cos;
				a[2] = 1 - alpha;
				
				make_direct_I (ca, cb);
			}
};

class Notch
: public RBJ
{
	public:
		template <class T>
		Notch (double f, double Q, T * ca, T * cb)
			: RBJ (f, Q)
			{
				b[0] = 1;
				b[1] = -2 * cos;
				b[2] = 1;
				
				a[0] = 1 + alpha;
				a[1] = -2 * cos;
				a[2] = 1 - alpha;
				
				make_direct_I (ca, cb);
			}
};

/* shelving and peaking dept. */

class PeakShelve
: public RBJ
{
	public:
		double A, beta;

	public:
		PeakShelve (double f, double Q, double dB)
			: RBJ (f, Q)
			{
				A = pow (10, dB * .025);
				double S = Q; /* slope */
				beta = sqrt ((A * A + 1) / S - (A - 1) * (A - 1));
			}
};

class LoShelve
: public PeakShelve
{
	public:
		template <class T>
		LoShelve (double f, double Q, double dB, T * ca, T * cb)
			: PeakShelve (f, Q, dB)
			{
				double Ap1 = A + 1, Am1 = A - 1;
				double beta_sin = beta * sin;

				b[0] =     A * (Ap1 - Am1 * cos + beta_sin);
				b[1] = 2 * A * (Am1 - Ap1 * cos);
				b[2] =     A * (Ap1 - Am1 * cos - beta_sin);

				a[0] =          Ap1 + Am1 * cos + beta_sin;
				a[1] = -2 *    (Am1 + Ap1 * cos);
				a[2] =          Ap1 + Am1 * cos - beta_sin;
				
				make_direct_I (ca, cb);
			}
};

class PeakingEQ
: public PeakShelve
{
	public:
		template <class T>
		PeakingEQ (double f, double Q, double dB, T * ca, T * cb)
			: PeakShelve (f, Q, dB)
			{
				b[0] = 1 + alpha * A;
				b[1] = -2 * cos;
				b[2] = 1 - alpha * A;

				a[0] = 1 + alpha / A;
				a[1] = -2 * cos;
				a[2] = 1 - alpha / A;
				
				make_direct_I (ca, cb);
			}
};

class HiShelve
: public PeakShelve
{
	public:
		template <class T>
		HiShelve (double f, double Q, double dB, T * ca, T * cb)
			: PeakShelve (f, Q, dB)
			{
				double Ap1 = A + 1, Am1 = A - 1;
				double beta_sin = beta * sin;

				b[0] =      A * (Ap1 + Am1 * cos + beta_sin);
				b[1] = -2 * A * (Am1 + Ap1 * cos);
				b[2] =      A * (Ap1 + Am1 * cos - beta_sin);

				a[0] =           Ap1 - Am1 * cos + beta_sin;
				a[1] =  2 *     (Am1 - Ap1 * cos);
				a[2] =           Ap1 - Am1 * cos - beta_sin;
				
				make_direct_I (ca, cb);
			}
};

} /* ~namespace RBJ */
} /* ~namespace DSP */

#endif /* _DSP_RBJ_H_ */
