/*****************************************************************************

        PolyphaseIir2Designer.cpp
        Author: Laurent de Soras, 2005

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if defined (_MSC_VER)
	#pragma warning (1 : 4130) // "'operator' : logical operation on address of string constant"
	#pragma warning (1 : 4223) // "nonstandard extension used : non-lvalue array converted to pointer"
	#pragma warning (1 : 4705) // "statement has no effect"
	#pragma warning (1 : 4706) // "assignment within conditional expression"
	#pragma warning (4 : 4786) // "identifier was truncated to '255' characters in the debug information"
	#pragma warning (4 : 4800) // "forcing value to bool 'true' or 'false' (performance warning)"
	#pragma warning (4 : 4355) // "'this' : used in base member initializer list"
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/def.h"
#include "hiir/fnc.h"
#include "hiir/PolyphaseIir2Designer.h"

#include <array>
#include <functional>

#include <cassert>
#include <cmath>



namespace hiir
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*
==============================================================================
Name: compute_nbr_coefs_from_proto
Description:
	Finds the minimum number of coefficients for a given filter specification
Input parameters:
	- attenuation: stopband attenuation, dB. > 0.
	- transition: normalized transition bandwith (% relative to Fs).
		Range ]0 ; 1/2[
Returns: Number of coefficients, > 0
Throws: Nothing
==============================================================================
*/

int	PolyphaseIir2Designer::compute_nbr_coefs_from_proto (double attenuation, double transition) noexcept
{
	assert (attenuation > 0);
	assert (transition > 0);
	assert (transition < 0.5);

	double         k;
	double         q;
	compute_transition_param (k, q, transition);
	const int      order     = compute_order (attenuation, q);
	const int      nbr_coefs = (order - 1) / 2;

	return nbr_coefs;
}



/*
==============================================================================
Name: compute_atten_from_order_tbw
Description:
	Compute the attenuation correspounding to a given number of coefficients
	and the transition bandwith.
Input parameters:
	- nbr_coefs: Number of desired coefficients. > 0.
	- transition: normalized transition bandwith (% relative to Fs).
		Range ]0 ; 1/2[
Returns: stopband attenuation, dB. > 0.
Throws: Nothing
==============================================================================
*/

double	PolyphaseIir2Designer::compute_atten_from_order_tbw (int nbr_coefs, double transition) noexcept
{
	assert (nbr_coefs > 0);
	assert (transition > 0);
	assert (transition < 0.5);

	double         k;
	double         q;
	compute_transition_param (k, q, transition);
	const int      order       = nbr_coefs * 2 + 1;
	const double   attenuation = compute_atten (q, order);

	return attenuation;
}



/*
==============================================================================
Name: compute_coefs
Description:
	Computes coefficients for a half-band polyphase IIR filter, function of a
	given stopband gain / transition bandwidth specification.
	Order is automatically calculated.
Input parameters:
	- attenuation: stopband attenuation, dB. > 0.
	- transition: normalized transition bandwith (% relative to Fs).
		Range ]0 ; 1/2[
Output parameters:
	- coef_arr: Coefficient list, must be large enough to store all the
		coefficients. Filter order = nbr_coefs * 2 + 1
Returns: number of coefficients
Throws: Nothing
==============================================================================
*/

int	PolyphaseIir2Designer::compute_coefs (double coef_arr [], double attenuation, double transition) noexcept
{
	assert (attenuation > 0);
	assert (transition > 0);
	assert (transition < 0.5);

	double         k;
	double         q;
	compute_transition_param (k, q, transition);

	// Computes number of required coefficients
	const int      order     = compute_order (attenuation, q);
	const int      nbr_coefs = (order - 1) / 2;

	// Coefficient calculation
	for (int index = 0; index < nbr_coefs; ++index)
	{
		coef_arr [index] = compute_coef (index, k, q, order);
	}

	return nbr_coefs;
}



/*
==============================================================================
Name: compute_coefs_spec_order_tbw
Description:
	Computes coefficients for a half-band polyphase IIR filter, function of a
	given transition bandwidth and desired filter order. Bandstop attenuation
	is set to the maximum value for these constraints.
Input parameters:
	- nbr_coefs: Number of desired coefficients. > 0.
	- transition: normalized transition bandwith (% relative to Fs).
		Range ]0 ; 1/2[
Output parameters:
	- coef_arr: Coefficient list, must be large enough to store all the
		coefficients.
Throws: Nothing
==============================================================================
*/

void	PolyphaseIir2Designer::compute_coefs_spec_order_tbw (double coef_arr [], int nbr_coefs, double transition) noexcept
{
	assert (nbr_coefs > 0);
	assert (transition > 0);
	assert (transition < 0.5);

	double         k;
	double         q;
	compute_transition_param (k, q, transition);
	const int      order = nbr_coefs * 2 + 1;

	// Coefficient calculation
	for (int index = 0; index < nbr_coefs; ++index)
	{
		coef_arr [index] = compute_coef (index, k, q, order);
	}
}



/*
==============================================================================
Name: compute_coefs_spec_order_gdly
Description:
	Computes the coefficients for a half-band polyphase IIR filter by
	specifying a phase delay and the desired filter order.
	It is also possible to specify the allowed ranges for attenuation and
	transition bandwidth.
	Phase delay range is constrained by the filter order and the measurement
	frequency, so the function may fail if the requirement cannot be met.
	Important note: the downsampler classes introduce an *advance* of 1/2
	output sample (half rate, so that's 1 input sample full rate). You have to
	take that into account if you're targetting the downsamplers by adding 1.0
	to the phase_delay value.
Input parameters:
	- nbr_coefs: Number of desired coefficients. > 0.
	- phase_delay: required phase delay in samples. The phase delay is measured
		at f_rel, and is reached within +/-prec samples. > 0.
	- f_rel: Normalised relative frequency for the phase delay measurement.
		Range is [0 ; 1[, where 1/2 is the Nyquist frequency for a full-rate
		(not decimated) filter.
	- prec: Precision of the phase delay requirement, in samples. > 0.
	- atten_lb: Lower bound for the stopband attenuation, in dB.
		Range ]0 ; atten_ub[
	- atten_ub: Upper bound for the stopband attenuation, in dB. > atten_lb.
	- trans_lb: Lower bound for the normalized transition bandwidth (relative
		to the sampling rate). Range ]0 ; trans_ub[
	- trans_ub: Upper bound for the normalized transition bandwidth (relative
		to the sampling rate). Range ]trans_lb ; 1/2[
Output parameters:
	- coef_arr: Coefficient list, must be large enough to store all the
		coefficients.
Input/output parameters:
	- attenuation_ptr: if the function succeeds and attenuation_ptr is not 0,
		the attenuation in dB for the designed filter is written on pointed
		location.
	- transition_ptr: if the function succeeds and transition_ptr is not 0,
		the normalized transition bandwidth is written there.
Returns:
	- 0 on success,
	- Negative number on failure, mainly because the constraints are too tight
		or the requirements unreachable. In this case, the output parameters
		should be considered as invalid.
Throws: Nothing
==============================================================================
*/

PolyphaseIir2Designer::ResCode	PolyphaseIir2Designer::compute_coefs_spec_order_pdly (double coef_arr [], double *attenuation_ptr, double *transition_ptr, int nbr_coefs, double phase_delay, double f_rel, double prec, double atten_lb, double atten_ub, double trans_lb, double trans_ub) noexcept
{
	using namespace std::placeholders;

	return compute_coefs_spec_order_delay (
		coef_arr, attenuation_ptr, transition_ptr, nbr_coefs, phase_delay,
		f_rel, prec, atten_lb, atten_ub, trans_lb, trans_ub,
		std::bind (compute_phase_delay, _1, _2, _3, false)
	);
}



/*
==============================================================================
Name: compute_coefs_spec_order_gdly
Description:
	Computes the coefficients for a half-band polyphase IIR filter by
	specifying a group delay and the desired filter order.
	It is also possible to specify the allowed ranges for attenuation and
	transition bandwidth.
	Group delay range is constrained by the filter order and the measurement
	frequency, so the function may fail if the requirement cannot be met.
	Important note: the downsampler classes introduce an *advance* of 1/2
	output sample (half rate, so that's 1 input sample full rate). You have to
	take that into account if you're targetting the downsamplers by adding 1.0
	to the group_delay value.
Input parameters:
	- nbr_coefs: Number of desired coefficients. > 0.
	- group_delay: required group delay in samples. The group delay is measured
		at f_rel, and is reached within +/-prec samples. > 0.
	- f_rel: Normalised relative frequency for the group delay measurement.
		Range is [0 ; 1[, where 1/2 is the Nyquist frequency for a full-rate
		(not decimated) filter.
	- prec: Precision of the group delay requirement, in samples. > 0.
	- atten_lb: Lower bound for the stopband attenuation, in dB.
		Range ]0 ; atten_ub[
	- atten_ub: Upper bound for the stopband attenuation, in dB. > atten_lb.
	- trans_lb: Lower bound for the normalized transition bandwidth (relative
		to the sampling rate). Range ]0 ; trans_ub[
	- trans_ub: Upper bound for the normalized transition bandwidth (relative
		to the sampling rate). Range ]trans_lb ; 1/2[
Output parameters:
	- coef_arr: Coefficient list, must be large enough to store all the
		coefficients.
Input/output parameters:
	- attenuation_ptr: if the function succeeds and attenuation_ptr is not 0,
		the attenuation in dB for the designed filter is written on pointed
		location.
	- transition_ptr: if the function succeeds and transition_ptr is not 0,
		the normalized transition bandwidth is written there.
Returns:
	- 0 on success,
	- Negative number on failure, mainly because the constraints are too tight
		or the requirements unreachable. In this case, the output parameters
		should be considered as invalid.
Throws: Nothing
==============================================================================
*/

PolyphaseIir2Designer::ResCode	PolyphaseIir2Designer::compute_coefs_spec_order_gdly (double coef_arr [], double *attenuation_ptr, double *transition_ptr, int nbr_coefs, double group_delay, double f_rel, double prec, double atten_lb, double atten_ub, double trans_lb, double trans_ub) noexcept
{
	using namespace std::placeholders;

	return compute_coefs_spec_order_delay (
		coef_arr, attenuation_ptr, transition_ptr, nbr_coefs, group_delay,
		f_rel, prec, atten_lb, atten_ub, trans_lb, trans_ub,
		std::bind (compute_group_delay, _1, _2, _3, false)
	);
}



/*
==============================================================================
Name: compute_phase_delay
Description:
	Computes the phase delay introduced by a complete filter at a specified
	frequency.
	The delay is given for a constant sampling rate between input and output.
Input parameters:
	- coef_arr: filter coefficient, as given by the designing functions.
	- nbr_coefs: Number of filter coefficients. > 0.
	- f_fs: frequency relative to the sampling rate, [0 ; 0.5].
	- ph_flag: set if filter is used in pi/2-phaser mode, in the form
		(a - z^-2) / (1 - az^-2)
Returns:
	The phase delay in samples, >= 0. In the phaser mode, the delay is
	negative and corresponds to the path for even coefficients (A0).
Throws: Nothing
==============================================================================
*/

double	PolyphaseIir2Designer::compute_phase_delay (const double coef_arr [], int nbr_coefs, double f_fs, bool ph_flag) noexcept
{
	assert (coef_arr != nullptr);
	assert (nbr_coefs > 0);
	assert (f_fs >= 0);
	assert (f_fs < 0.5);

	auto           dly_arr = std::array <double, 2> { 0, 1 };
	for (int k = 0; k < nbr_coefs; ++k)
	{
		const auto     dly =
			compute_unit_phase_delay (coef_arr [k], f_fs, ph_flag);
		dly_arr [k & 1] += dly;
	}

	const auto     com       = dly_arr [0];     // Common delay (the even path)
	auto           dly_final = com;

	if (! ph_flag && f_fs > 0)
	{
		const auto     w   = 2 * hiir::PI * f_fs;
		const auto     phi = (dly_arr [1] - com) * w;
		assert (std::abs (phi) < hiir::PI);
		const auto     re  = cos (phi) + 1;      // Complex sum of both paths
		const auto     im  = sin (phi) + 0;
		const auto     dif = atan2 (im, re) / w; // Odd path relative to the even
		dly_final += dif;
	}

	return dly_final;
}



/*
==============================================================================
Name: compute_group_delay
Description:
	Computes the group delay introduced by a complete filter at a specified
	frequency.
	The delay is given for a constant sampling rate between input and output.
	Note that the result is accurate only in the passband, where A0 and A1
	outputs of are almost equal.
Input parameters:
	- coef_arr: filter coefficient, as given by the designing functions.
		Actually only even coefficients (A0 part) are used.
	- nbr_coefs: Number of filter coefficients. > 0.
	- f_fs: frequency relative to the sampling rate, [0 ; 0.5].
	- ph_flag: set if filter is used in pi/2-phaser mode, in the form
		(a - z^-2) / (1 - az^-2)
Returns:
	The group delay in samples, >= 0.
Throws: Nothing
==============================================================================
*/

double	PolyphaseIir2Designer::compute_group_delay (const double coef_arr [], int nbr_coefs, double f_fs, bool ph_flag) noexcept
{
	assert (coef_arr != nullptr);
	assert (nbr_coefs > 0);
	assert (f_fs >= 0);
	assert (f_fs < 0.5);

	double         dly_total = 0;
	for (int k = 0; k < nbr_coefs; k += 2)
	{
		const auto     dly =
			compute_unit_group_delay (coef_arr [k], f_fs, ph_flag);
		dly_total += dly;
	}

	return dly_total;
}



/*
==============================================================================
Name: compute_unit_phase_delay
Description:
	Computes the phase delay introduced by a single filtering unit at a
	specified frequency.
	The delay is given for a constant sampling rate between input and output.
Input parameters:
	- a: coefficient for the cell, [0 ; 1[
	- f_fs: frequency relative to the sampling rate, [0 ; 0.5].
	- ph_flag: set if filtering unit is used in pi/2-phaser mode, in the form
		(a - z^-2) / (1 - az^-2)
Returns:
	The phase delay in samples, positive when ph_flag is false, negative in
	phaser mode.
Throws: Nothing
==============================================================================
*/

double	PolyphaseIir2Designer::compute_unit_phase_delay (double a, double f_fs, bool ph_flag) noexcept
{
	assert (a >= 0);
	assert (a < 1);
	assert (f_fs >= 0);
	assert (f_fs < 0.5);

	const auto     w  = 2 * hiir::PI * f_fs;
	const auto     c  = cos (w);
	const auto     s  = sin (w);
	const auto     u  = (ph_flag) ? -1 : 1;
	const auto     ac = (a + u) * c;
	const auto     as = (a - u) * s;
	const auto     x  = u * (ac * ac - as * as);
	const auto     y  = u * 2 * ac * as;
	auto           ph = -atan2 (y, x);
	if (ph * u < 0)
	{
		ph += u * 2 * hiir::PI;
	}
	const auto     dly = (w > 0) ? ph / w : -2 * u * (a - u) / (a + u);

	return dly;
}



/*
==============================================================================
Name: compute_unit_group_delay
Description:
	Computes the group delay introduced by a single filtering unit at a
	specified frequency.
	The delay is given for a constant sampling rate between input and output.
	To compute the group delay of a complete filter, add the group delays
	of all the units in A0 (z).
Input parameters:
	- a: coefficient for the cell, [0 ; 1[
	- f_fs: frequency relative to the sampling rate, [0 ; 0.5].
	- ph_flag: set if filtering unit is used in pi/2-phaser mode, in the form
		(a - z^-2) / (1 - az^-2)
Returns:
	The group delay in samples, >= 0.
Throws: Nothing
==============================================================================
*/

double	PolyphaseIir2Designer::compute_unit_group_delay (double a, double f_fs, bool ph_flag) noexcept
{
	assert (a >= 0);
	assert (a < 1);
	assert (f_fs >= 0);
	assert (f_fs < 0.5);

	const auto     w   = 2 * hiir::PI * f_fs;
	const auto     a2  = a * a;
	const auto     sig = (ph_flag) ? -2 : 2;
	const auto     dly = 2 * (1 - a2) / (a2 + sig * a * cos (2 * w) + 1);

	return dly;
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	PolyphaseIir2Designer::compute_transition_param (double &k, double &q, double transition) noexcept
{
	assert (transition > 0);
	assert (transition < 0.5);

	k  = tan ((1 - transition * 2) * hiir::PI / 4);
	k *= k;
	assert (k < 1);
	assert (k > 0);
	double         kksqrt = pow (1 - k * k, 0.25);
	const double   e = 0.5 * (1 - kksqrt) / (1 + kksqrt);
	const double   e2 = e * e;
	const double   e4 = e2 * e2;
	q = e * (1 + e4 * (2 + e4 * (15 + 150 * e4)));
	assert (q > 0);
}



int	PolyphaseIir2Designer::compute_order (double attenuation, double q) noexcept
{
	assert (attenuation > 0);
	assert (q > 0);

	const double   attn_p2 = pow (10.0, -attenuation / 10);
	const double   a       = attn_p2 / (1 - attn_p2);
	int            order   = hiir::ceil_int (log (a * a / 16) / log (q));
	if ((order & 1) == 0)
	{
		++ order;
	}
	if (order == 1)
	{
		order = 3;
	}

	return order;
}



double	PolyphaseIir2Designer::compute_atten (double q, int order) noexcept
{
	assert (q > 0);
	assert (order > 0);
	assert ((order & 1) == 1);

	const double   a           = 4 * exp (order * 0.5 * log (q));
	assert (a != -1.0);
	const double   attn_p2     = a / (1 + a);
	const double   attenuation = -10 * log10 (attn_p2);
	assert (attenuation > 0);

	return attenuation;
}



double	PolyphaseIir2Designer::compute_coef (int index, double k, double q, int order) noexcept
{
	assert (index >= 0);
	assert (index * 2 < order);

	const int      c    = index + 1;
	const double   num  = compute_acc_num (q, order, c) * pow (q, 0.25);
	const double   den  = compute_acc_den (q, order, c) + 0.5;
	const double   ww   = num / den;
	const double   wwsq = ww * ww;

	const double   x    = sqrt ((1 - wwsq * k) * (1 - wwsq / k)) / (1 + wwsq);
	const double   coef = (1 - x) / (1 + x);

	return coef;
}



double	PolyphaseIir2Designer::compute_acc_num (double q, int order, int c) noexcept
{
	assert (c >= 1);
	assert (c < order * 2);

	int            i   = 0;
	int            j   = 1;
	double         acc = 0;
	double         q_ii1;
	do
	{
		q_ii1  = hiir::ipowp (q, i * (i + 1));
		q_ii1 *= sin ((i * 2 + 1) * c * hiir::PI / order) * j;
		acc   += q_ii1;

		j = -j;
		++i;
	}
	while (fabs (q_ii1) > 1e-100);

	return acc;
}



double	PolyphaseIir2Designer::compute_acc_den (double q, int order, int c) noexcept
{
	assert (c >= 1);
	assert (c < order * 2);

	int            i   =  1;
	int            j   = -1;
	double         acc =  0;
	double         q_i2;
	do
	{
		q_i2  = hiir::ipowp (q, i * i);
		q_i2 *= cos (i * 2 * c * hiir::PI / order) * j;
		acc  += q_i2;

		j = -j;
		++i;
	}
	while (fabs (q_i2) > 1e-100);

	return acc;
}



template <typename F>
PolyphaseIir2Designer::ResCode	PolyphaseIir2Designer::compute_coefs_spec_order_delay (double coef_arr [], double *attenuation_ptr, double *transition_ptr, int nbr_coefs, double delay, double f_rel, double prec, double atten_lb, double atten_ub, double trans_lb, double trans_ub, F compute_delay) noexcept
{
	assert (nbr_coefs > 0);
	assert (nbr_coefs <= _max_order);
	assert (delay > 0);
	assert (f_rel >= 0);
	assert (f_rel < 1);
	assert (prec > 0);
	assert (atten_lb > 0);
	assert (atten_lb < atten_ub);
	assert (trans_lb > 0);
	assert (trans_lb < trans_ub);
	assert (trans_ub < 0.5);

	ResCode        ret_val = ResCode_OK;

	double         lb_tbw = trans_lb;
	double         ub_tbw = trans_ub;
	std::array <double, _max_order>  lb_coef_arr;
	std::array <double, _max_order>  ub_coef_arr;

	compute_coefs_spec_order_tbw (ub_coef_arr.data (), nbr_coefs, ub_tbw);
	compute_coefs_spec_order_tbw (lb_coef_arr.data (), nbr_coefs, lb_tbw);

	double   ub_gdly = compute_delay (ub_coef_arr.data (), nbr_coefs, f_rel);
	double   lb_gdly = compute_delay (lb_coef_arr.data (), nbr_coefs, f_rel);

	// Checks if the group delay and transition bandwidth requirements
	// could be met
	if ((ub_gdly - delay) * (delay - lb_gdly) <= 0)
	{
		ret_val = ResCode_FAIL_GD_TBW;
	}

	double         rs_attn = 0;
	double         rs_tbw  = 0;

	if (ret_val == ResCode_OK)
	{
		// Simple bisection method
		const int      max_it    = 1000;
		int            nbr_it    = 0;
		double         rs_gdly   = 0;
		bool           conv_flag = false;
		do
		{
			rs_tbw  = (ub_tbw + lb_tbw) * 0.5;
			rs_attn = compute_atten_from_order_tbw (nbr_coefs, rs_tbw);
			compute_coefs_spec_order_tbw (coef_arr, nbr_coefs, rs_tbw);
			rs_gdly = compute_delay (coef_arr, nbr_coefs, f_rel);

			if ((delay - lb_gdly) * (delay - rs_gdly) < 0)
			{
				ub_tbw  = rs_tbw;
//				ub_gdly = rs_gdly; // ub_gdly not used in the loop actually
			}
			else
			{
				lb_tbw  = rs_tbw;
				lb_gdly = rs_gdly;
			}

			++ nbr_it;
			conv_flag = (fabs (rs_gdly - delay) > prec);
		}
		while (conv_flag && nbr_it < max_it);

		// Checks convergence
		if (nbr_it >= max_it && ! conv_flag)
		{
			ret_val = ResCode_FAIL_CONV;
		}

		// Checks the attenuation requirement
		else if (   rs_attn < atten_lb
		         || rs_attn > atten_ub)
		{
			ret_val = ResCode_FAIL_ATTEN;
		}
	}

	if (ret_val == ResCode_OK)
	{
		if (attenuation_ptr != nullptr)
		{
			*attenuation_ptr = rs_attn;
		}
		if (transition_ptr != nullptr)
		{
			*transition_ptr = rs_tbw;
		}
	}

	return ret_val;
}



}	// namespace hiir



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
