/*****************************************************************************

        HalfBandSse.hpp
        Author: Laurent de Soras, 2021

Vector layout:
- 0: Odd coefs , pass 2 (-> output)
- 1: Even coefs, pass 2 (-> output)
- 2: Odd coefs , pass 1 (<- input)
- 3: Even coefs, pass 1 (<- input)

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (hiir_HalfBandSse_CODEHEADER_INCLUDED)
#define hiir_HalfBandSse_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/StageProcSseV4.h"

#include <mmintrin.h>

#include <cassert>



namespace hiir
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int NC>
constexpr int 	HalfBandSse <NC>::_nbr_chn;
template <int NC>
constexpr int 	HalfBandSse <NC>::NBR_COEFS;
template <int NC>
constexpr double	HalfBandSse <NC>::_delay;



/*
==============================================================================
Name: ctor
Throws: Nothing
==============================================================================
*/

template <int NC>
HalfBandSse <NC>::HalfBandSse () noexcept
:	_filter ()
,	_prev (0)
,	_phase (0)
{
	for (int phase = 0; phase < _nbr_phases; ++phase)
	{
		for (int i = 0; i < _nbr_stages + 1; ++i)
		{
			_mm_store_ps (_filter [phase] [i]._coef, _mm_setzero_ps ());
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
void	HalfBandSse <NC>::set_coefs (const double coef_arr []) noexcept
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
float	HalfBandSse <NC>::process_sample (float input) noexcept
{
	const auto     y   = process_2_paths (input);
	const auto     low = y [0] + y [1];

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
void	HalfBandSse <NC>::process_block (float out_ptr [], const float in_ptr [], long nbr_spl) noexcept
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
float	HalfBandSse <NC>::process_sample_hpf (float input) noexcept
{
	const auto     y   = process_2_paths (input);
	const auto     hi  = y [0] - y [1];

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
void	HalfBandSse <NC>::process_block_hpf (float out_ptr [], const float in_ptr [], long nbr_spl) noexcept
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
void	HalfBandSse <NC>::process_sample_split (float &low, float &hi, float input) noexcept
{
	const auto     y   = process_2_paths (input);
	low = y [0] + y [1];
	hi  = y [0] - y [1];
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
void	HalfBandSse <NC>::process_block_split (float out_l_ptr [], float out_h_ptr [], const float in_ptr [], long nbr_spl) noexcept
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
void	HalfBandSse <NC>::clear_buffers () noexcept
{
	for (int phase = 0; phase < _nbr_phases; ++phase)
	{
		for (int i = 0; i < _nbr_stages + 1; ++i)
		{
			_mm_store_ps (_filter [phase] [i]._mem, _mm_setzero_ps ());
		}
	}

	_prev  = 0;
	_phase = 0;
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int NC>
constexpr int	HalfBandSse <NC>::_stage_width;
template <int NC>
constexpr int	HalfBandSse <NC>::_nbr_stages;
template <int NC>
constexpr int	HalfBandSse <NC>::_nbr_phases;
template <int NC>
constexpr int	HalfBandSse <NC>::_coef_shift;



template <int NC>
void	HalfBandSse <NC>::set_single_coef (int index, double coef) noexcept
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
// Returns { even coefs, odd coefs }
template <int NC>
std::array <float, 2>	HalfBandSse <NC>::process_2_paths (float input) noexcept
{
	StageDataSse * filter_ptr = _filter [_phase].data ();

	const auto     spl_in  = _mm_load_ss (&input);
	const auto     prev    = _mm_load_ss (&_prev);
	const auto     comb    = _mm_unpacklo_ps (prev, spl_in);
	const auto     spl_mid = _mm_load_ps (filter_ptr [_nbr_stages]._mem);
	constexpr auto shuf    = (2 << 0) | (3 << 2) | (0 << 4) | (1 << 6);
	auto           y       = _mm_shuffle_ps (spl_mid, comb, shuf);

	auto           mem     = _mm_load_ps (filter_ptr [0]._mem);

	StageProcSseV4 <_nbr_stages>::process_sample_pos (filter_ptr, y, mem);

	_mm_store_ps (filter_ptr [_nbr_stages]._mem, y);

	_prev  = input;
	_phase = 1 - _phase;

	y = _mm_mul_ps (y, _mm_set1_ps (0.5f));
	const auto     even = _mm_cvtss_f32 (_mm_shuffle_ps (y, y, 1));
	const auto     odd  = _mm_cvtss_f32 (y);

	return { even, odd };
}



template <int NC>
template <typename FL, typename FH>
long	HalfBandSse <NC>::process_block_quad (float out_l_ptr [], float out_h_ptr [], const float in_ptr [], long nbr_spl, FL fnc_l, FH fnc_h) noexcept
{
	assert (_phase == 0);

	const auto     half   = _mm_set1_ps (0.5f);
	constexpr auto shuf_y = (2 << 0) | (3 << 2);
	constexpr auto shuf_1 = shuf_y | (0 << 4) | (1 << 6);
	constexpr auto shuf_2 = shuf_y | (1 << 4) | (2 << 6);
	constexpr auto shuf_3 = shuf_y | (2 << 4) | (3 << 6);

	const long     n4     = nbr_spl & ~(4-1);
	auto           prev   = _mm_load_ss (&_prev);
	auto           y_2    = _mm_load_ps (_filter [0] [_nbr_stages]._mem);
	auto           y_3    = _mm_load_ps (_filter [1] [_nbr_stages]._mem);
	for (long pos = 0; pos < n4; pos += 4)
	{
		const auto     x      = _mm_loadu_ps (in_ptr + pos);

		const auto     comb_0 = _mm_unpacklo_ps (prev, x);
		auto           y_0    = _mm_shuffle_ps (y_2, comb_0, shuf_1);
		auto           mem_0  = _mm_load_ps (_filter [0] [0]._mem);
		StageProcSseV4 <_nbr_stages>::process_sample_pos (_filter [0].data (), y_0, mem_0);
		_mm_store_ps (_filter [0] [_nbr_stages]._mem, y_0);

		auto           y_1    = _mm_shuffle_ps (y_3, x, shuf_1);
		auto           mem_1  = _mm_load_ps (_filter [1] [0]._mem);
		StageProcSseV4 <_nbr_stages>::process_sample_pos (_filter [1].data (), y_1, mem_1);
		_mm_store_ps (_filter [1] [_nbr_stages]._mem, y_1);

		               y_2    = _mm_shuffle_ps (y_0, x, shuf_2);
		auto           mem_2  = _mm_load_ps (_filter [0] [0]._mem);
		StageProcSseV4 <_nbr_stages>::process_sample_pos (_filter [0].data (), y_2, mem_2);
		_mm_store_ps (_filter [0] [_nbr_stages]._mem, y_2);

		               y_3    = _mm_shuffle_ps (y_1, x, shuf_3);
		auto           mem_3  = _mm_load_ps (_filter [1] [0]._mem);
		StageProcSseV4 <_nbr_stages>::process_sample_pos (_filter [1].data (), y_3, mem_3);
		_mm_store_ps (_filter [1] [_nbr_stages]._mem, y_3);

		prev = _mm_shuffle_ps (x, x, 3);

		const auto     u_01 = _mm_unpacklo_ps (y_0, y_1); // o0, o1, e0, e1
		const auto     u_23 = _mm_unpacklo_ps (y_2, y_3); // o2, o3, e2, e3
		const auto     odd  = _mm_movelh_ps (u_01, u_23);
		const auto     even = _mm_movehl_ps (u_23, u_01);
		fnc_l (out_l_ptr + pos, even, odd, half);
		fnc_h (out_h_ptr + pos, even, odd, half);
	}
	_prev = _mm_cvtss_f32 (prev);

	return n4;
}



template <int NC>
void	HalfBandSse <NC>::store_low (float *ptr, __m128 even, __m128 odd, __m128 half) noexcept
{
	const auto     low  = _mm_mul_ps (_mm_add_ps (even, odd), half);
	_mm_storeu_ps (ptr, low);
}



template <int NC>
void	HalfBandSse <NC>::store_high (float *ptr, __m128 even, __m128 odd, __m128 half) noexcept
{
	const auto     high = _mm_mul_ps (_mm_sub_ps (even, odd), half);
	_mm_storeu_ps (ptr, high);
}



}  // namespace hiir



#endif   // hiir_HalfBandSse_CODEHEADER_INCLUDED

#undef hiir_HalfBandSse_CURRENT_CODEHEADER



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
