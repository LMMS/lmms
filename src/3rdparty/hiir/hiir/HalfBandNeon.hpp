/*****************************************************************************

        HalfBandNeon.hpp
        Author: Laurent de Soras, 2016

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (hiir_HalfBandNeon_CODEHEADER_INCLUDED)
#define hiir_HalfBandNeon_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/fnc_neon.h"
#include "hiir/StageProcNeonV4.h"

#include <cassert>



namespace hiir
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int NC>
constexpr int 	HalfBandNeon <NC>::_nbr_chn;
template <int NC>
constexpr int 	HalfBandNeon <NC>::NBR_COEFS;
template <int NC>
constexpr double	HalfBandNeon <NC>::_delay;



/*
==============================================================================
Name: ctor
Throws: Nothing
==============================================================================
*/

template <int NC>
HalfBandNeon <NC>::HalfBandNeon () noexcept
:	_filter ()
,	_prev (0)
,	_phase (0)
{
	for (int phase = 0; phase < _nbr_phases; ++phase)
	{
		for (int i = 0; i < _nbr_stages + 1; ++i)
		{
			storea (_filter [phase] [i]._coef, vdupq_n_f32 (0));
		}
	}
	for (int i = NBR_COEFS; i < _nbr_stages * _stage_width; ++i)
	{
		set_single_coef (i, 1);
	}

	clear_buffers ();
}



/*
==============================================================================
Name: set_coefs
Description:
	Sets filter coefficients. Generate them with the PolyphaseIir2Designer
	class.
	Call this function before doing any processing.
Input parameters:
	- coef_arr: Array of coefficients. There should be as many coefficients as
		mentioned in the class template parameter.
Throws: Nothing
==============================================================================
*/

template <int NC>
void	HalfBandNeon <NC>::set_coefs (const double coef_arr []) noexcept
{
	assert (coef_arr != nullptr);

	for (int i = 0; i < NBR_COEFS; ++i)
	{
		set_single_coef (i, coef_arr [i]);
	}
}



/*
==============================================================================
Name: process_sample
Description:
	Performs a half-band low-pass filtering on a sample.
Input parameters:
	- in: sample from the stream to be filtered.
Returns: filtered sample.
Throws: Nothing
==============================================================================
*/

template <int NC>
float	HalfBandNeon <NC>::process_sample (float input) noexcept
{
	auto           y    = process_2_paths (input);
	const auto     even = vget_lane_f32 (y, 1);
	const auto     odd  = vget_lane_f32 (y, 0);
	const auto     low  = even + odd;

	return low;
}



/*
==============================================================================
Name: process_block
Description:
	Performs a half-band low-pass filtering on a block of samples.
	Input and output blocks may overlap, see assert() for details.
Input parameters:
	- in_ptr: Input array, containing nbr_spl samples.
	- nbr_spl: Number of samples to output, > 0
Output parameters:
	- out_ptr: Array for the output samples, capacity: nbr_spl samples.
Throws: Nothing
==============================================================================
*/

template <int NC>
void	HalfBandNeon <NC>::process_block (float out_ptr [], const float in_ptr [], long nbr_spl) noexcept
{
	assert (in_ptr  != nullptr);
	assert (out_ptr != nullptr);
	assert (out_ptr <= in_ptr || out_ptr >= in_ptr + nbr_spl);
	assert (nbr_spl > 0);

	if (_phase != 0)
	{
		out_ptr [0] = process_sample (in_ptr [0]);
		++ out_ptr;
		++ in_ptr;
		-- nbr_spl;
	}

	if (nbr_spl > 0)
	{
		const long     n4 = process_block_quad (
			out_ptr, nullptr, in_ptr, nbr_spl, store_low, bypass
		);

		for (long pos = n4; pos < nbr_spl; ++pos)
		{
			out_ptr [pos] = process_sample (in_ptr [pos]);
		}
	}
}



/*
==============================================================================
Name: process_sample_hpf
Description:
	Performs a half-band high-pass filtering on a sample.
Input parameters:
	- in: sample from the stream to be filtered.
Returns: filtered sample.
Throws: Nothing
==============================================================================
*/

template <int NC>
float	HalfBandNeon <NC>::process_sample_hpf (float input) noexcept
{
	auto           y    = process_2_paths (input);
	const auto     even = vget_lane_f32 (y, 1);
	const auto     odd  = vget_lane_f32 (y, 0);
	const auto     hi   = even - odd;

	return hi;
}



/*
==============================================================================
Name: process_block_hpf
Description:
	Performs a half-band high-pass filtering on a block of samples.
	Input and output blocks may overlap, see assert() for details.
Input parameters:
	- in_ptr: Input array, containing nbr_spl samples.
	- nbr_spl: Number of samples to output, > 0
Output parameters:
	- out_ptr: Array for the output samples, capacity: nbr_spl samples.
Throws: Nothing
==============================================================================
*/

template <int NC>
void	HalfBandNeon <NC>::process_block_hpf (float out_ptr [], const float in_ptr [], long nbr_spl) noexcept
{
	assert (in_ptr  != nullptr);
	assert (out_ptr != nullptr);
	assert (out_ptr <= in_ptr || out_ptr >= in_ptr + nbr_spl);
	assert (nbr_spl > 0);

	if (_phase != 0)
	{
		out_ptr [0] = process_sample_hpf (in_ptr [0]);
		++ out_ptr;
		++ in_ptr;
		-- nbr_spl;
	}

	if (nbr_spl > 0)
	{
		const long     n4 = process_block_quad (
			nullptr, out_ptr, in_ptr, nbr_spl, bypass, store_high
		);

		for (long pos = n4; pos < nbr_spl; ++pos)
		{
			out_ptr [pos] = process_sample_hpf (in_ptr [pos]);
		}
	}
}



/*
==============================================================================
Name: process_sample_split
Description:
	Splits (spectrum-wise) in half a sample from a stream. Both part are
	results of a low-pass and a high-pass filtering, equivalent to the output
	of process_sample() and process_sample_hpf().
Input parameters:
	- in: sample from the stream to be filtered.
Output parameters:
	- low: output sample, lower part of the spectrum.
	- high: output sample, higher part of the spectrum.
Throws: Nothing
==============================================================================
*/

template <int NC>
void	HalfBandNeon <NC>::process_sample_split (float &low, float &hi, float input) noexcept
{
	auto           y    = process_2_paths (input);
	const auto     even = vget_lane_f32 (y, 1);
	const auto     odd  = vget_lane_f32 (y, 0);
	low = even + odd;
	hi  = even - odd;
}



/*
==============================================================================
Name: process_block_split
Description:
	Splits (spectrum-wise) in half a block of samples. Both part are
	results of a low-pass and a high-pass filtering, equivalent to the output
	of process_sample() and process_sample_hpf().
	Input and output blocks may overlap, see assert() for details.
Input parameters:
	- in_ptr: Input array, containing nbr_spl samples.
	- nbr_spl: Number of samples to output, > 0
Output parameters:
	- out_l_ptr: Array for the output samples, lower part of the spectrum
		Capacity: nbr_spl samples.
	- out_h_ptr: Array for the output samples, higher part of the spectrum.
		Capacity: nbr_spl samples.
Throws: Nothing
==============================================================================
*/

template <int NC>
void	HalfBandNeon <NC>::process_block_split (float out_l_ptr [], float out_h_ptr [], const float in_ptr [], long nbr_spl) noexcept
{
	assert (out_l_ptr != nullptr);
	assert (out_h_ptr != nullptr);
	assert (in_ptr != nullptr);
	assert (out_l_ptr <= in_ptr || out_l_ptr >= in_ptr + nbr_spl);
	assert (out_h_ptr <= in_ptr || out_h_ptr >= in_ptr + nbr_spl);
	assert (out_l_ptr + nbr_spl <= out_h_ptr || out_h_ptr + nbr_spl <= out_l_ptr);
	assert (nbr_spl > 0);

	if (_phase != 0)
	{
		process_sample_split (out_l_ptr [0], out_h_ptr [0], in_ptr [0]);
		++ out_l_ptr;
		++ out_h_ptr;
		++ in_ptr;
		-- nbr_spl;
	}

	if (nbr_spl > 0)
	{
		const long     n4 = process_block_quad (
			out_l_ptr, out_h_ptr, in_ptr, nbr_spl, store_low, store_high
		);

		for (long pos = n4; pos < nbr_spl; ++pos)
		{
			process_sample_split (out_l_ptr [pos], out_h_ptr [pos], in_ptr [pos]);
		}
	}
}



/*
==============================================================================
Name: clear_buffers
Description:
	Clears filter memory, as if it processed silence since an infinite amount
	of time.
Throws: Nothing
==============================================================================
*/

template <int NC>
void	HalfBandNeon <NC>::clear_buffers () noexcept
{
   for (int phase = 0; phase < _nbr_phases; ++phase)
   {
		for (int i = 0; i < _nbr_stages + 1; ++i)
		{
			storea (_filter [phase] [i]._mem, vdupq_n_f32 (0));
		}
	}
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int NC>
constexpr int	HalfBandNeon <NC>::_stage_width;
template <int NC>
constexpr int	HalfBandNeon <NC>::_nbr_stages;
template <int NC>
constexpr int	HalfBandNeon <NC>::_nbr_phases;
template <int NC>
constexpr int	HalfBandNeon <NC>::_coef_shift;



template <int NC>
void	HalfBandNeon <NC>::set_single_coef (int index, double coef) noexcept
{
	assert (index >= 0);
	assert (index < _nbr_stages * _stage_width);

	const int      stage = (index / _stage_width) + 1;
	const int      pos   = (index ^ _coef_shift) & (_stage_width - 1);
	for (int phase = 0; phase < _nbr_phases; ++phase)
	{
		_filter [phase] [stage]._coef [pos] = DataType (coef);
	}
}



// Shared processing function, outputs both paths of the all-pass filter pair.
// y [0] = Odd coefs
// y [1] = Even coefs
template <int NC>
float32x2_t	HalfBandNeon <NC>::process_2_paths (float input) noexcept
{
	const auto     filter_ptr = _filter [_phase].data ();

	const float32x2_t comb = vset_lane_f32 (input, vdup_n_f32 (_prev), 1);
	const float32x2_t mid  =
#if ! defined (__BYTE_ORDER__) || (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
		// Requires a little-endian architecture, which is generally the case
		load2a (&filter_ptr [_nbr_stages]._mem [2]);
#else
		// Safe on any platform, but possibly slower.
		vget_high_f32 (load4a (filter_ptr [_nbr_stages]._mem));
#endif
	float32x4_t       y   = vcombine_f32 (mid, comb);
	float32x4_t       mem = load4a (filter_ptr [0]._mem);

	StageProcNeonV4 <_nbr_stages>::process_sample_pos (filter_ptr, y, mem);

	_prev  = input;
	_phase = 1 - _phase;

	return vget_low_f32 (y) * vdup_n_f32 (0.5f);
}



template <int NC>
template <typename FL, typename FH>
long	HalfBandNeon <NC>::process_block_quad (float out_l_ptr [], float out_h_ptr [], const float in_ptr [], long nbr_spl, FL fnc_l, FH fnc_h) noexcept
{
	assert (_phase == 0);

	const auto     half   = vdupq_n_f32 (0.5f);

	const long     n4     = nbr_spl & ~(4-1);
	auto           prev   = _prev;
	auto           y_2    = load4a (_filter [0] [_nbr_stages]._mem);
	auto           y_3    = load4a (_filter [1] [_nbr_stages]._mem);
	for (long pos = 0; pos < n4; pos += 4)
	{
		const auto     x      = load4u (in_ptr + pos);

		const auto     comb_0 = float32x2_t { prev, vgetq_lane_f32 (x, 0) };
		auto           y_0    = vcombine_f32 (vget_high_f32 (y_2), comb_0);
		auto           mem_0  = load4a (_filter [0] [0]._mem);
		StageProcNeonV4 <_nbr_stages>::process_sample_pos (_filter [0].data (), y_0, mem_0);
		storea (_filter [0] [_nbr_stages]._mem, y_0);

		auto           y_1    = vcombine_f32 (vget_high_f32 (y_3), vget_low_f32 (x));
		auto           mem_1  = load4a (_filter [1] [0]._mem);
		StageProcNeonV4 <_nbr_stages>::process_sample_pos (_filter [1].data (), y_1, mem_1);
		storea (_filter [1] [_nbr_stages]._mem, y_1);

		const auto     comb_2 = float32x2_t { vgetq_lane_f32 (x, 1), vgetq_lane_f32 (x, 2) };
		               y_2    = vcombine_f32 (vget_high_f32 (y_0), comb_2);
		auto           mem_2  = load4a (_filter [0] [0]._mem);
		StageProcNeonV4 <_nbr_stages>::process_sample_pos (_filter [0].data (), y_2, mem_2);
		storea (_filter [0] [_nbr_stages]._mem, y_2);

		               y_3    = vcombine_f32 (vget_high_f32 (y_1), vget_high_f32 (x));
		auto           mem_3  = load4a (_filter [1] [0]._mem);
		StageProcNeonV4 <_nbr_stages>::process_sample_pos (_filter [1].data (), y_3, mem_3);
		storea (_filter [1] [_nbr_stages]._mem, y_3);

		prev = vgetq_lane_f32 (x, 3);

		const auto     u_02 = vcombine_f32 (vget_low_f32 (y_0), vget_low_f32 (y_2));
		const auto     u_13 = vcombine_f32 (vget_low_f32 (y_1), vget_low_f32 (y_3));
		const auto     both = vtrnq_f32 (u_02, u_13);
		const auto     odd  = both.val [0];
		const auto     even = both.val [1];
		fnc_l (out_l_ptr + pos, even, odd, half);
		fnc_h (out_h_ptr + pos, even, odd, half);
	}
	_prev = prev;

	return n4;
}



template <int NC>
void	HalfBandNeon <NC>::store_low (float *ptr, float32x4_t even, float32x4_t odd, float32x4_t half) noexcept
{
	const auto     low  = (even + odd) * half;
	storeu (ptr, low);
}



template <int NC>
void	HalfBandNeon <NC>::store_high (float *ptr, float32x4_t even, float32x4_t odd, float32x4_t half) noexcept
{
	const auto     high = (even - odd) * half;
	storeu (ptr, high);
}



}  // namespace hiir



#endif   // hiir_HalfBandNeon_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
