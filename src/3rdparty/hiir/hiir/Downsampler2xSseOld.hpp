/*****************************************************************************

        Downsampler2xSseOld.hpp
        Author: Laurent de Soras, 2005

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if defined (hiir_Downsampler2xSseOld_CURRENT_CODEHEADER)
	#error Recursive inclusion of Downsampler2xSseOld code header.
#endif
#define hiir_Downsampler2xSseOld_CURRENT_CODEHEADER

#if ! defined (hiir_Downsampler2xSseOld_CODEHEADER_INCLUDED)
#define hiir_Downsampler2xSseOld_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/StageProcSseV4.h"

#include <xmmintrin.h>

#include <cassert>



namespace hiir
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int NC>
constexpr int	Downsampler2xSseOld <NC>::_nbr_chn;
template <int NC>
constexpr int	Downsampler2xSseOld <NC>::NBR_COEFS;
template <int NC>
constexpr double	Downsampler2xSseOld <NC>::_delay;



/*
==============================================================================
Name: ctor
Throws: Nothing
==============================================================================
*/

template <int NC>
Downsampler2xSseOld <NC>::Downsampler2xSseOld () noexcept
:	_filter ()
{
	for (int i = 0; i < _nbr_stages + 1; ++i)
	{
		_mm_store_ps (_filter [i]._coef, _mm_setzero_ps ());
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
void	Downsampler2xSseOld <NC>::set_coefs (const double coef_arr []) noexcept
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
	Downsamples (x2) one pair of samples, to generate one output sample.
Input parameters:
	- in_ptr: pointer on the two samples to decimate
Returns: Samplerate-reduced sample.
Throws: Nothing
==============================================================================
*/

template <int NC>
float	Downsampler2xSseOld <NC>::process_sample (const float in_ptr [2]) noexcept
{
	assert (in_ptr != nullptr);

	// Combines two input samples and two mid-processing data
	const auto     spl_in  = _mm_loadu_ps (in_ptr);
	const auto     spl_mid = _mm_load_ps (_filter [_nbr_stages]._mem);
	constexpr auto shuf    = (2 << 0) | (3 << 2) | (0 << 4) | (1 << 6);
	auto           y       = _mm_shuffle_ps (spl_mid, spl_in, shuf);

	auto           mem     = _mm_load_ps (_filter [0]._mem);

	// Processes each stage
	StageProcSseV4 <_nbr_stages>::process_sample_pos (_filter.data (), y, mem);

	_mm_store_ps (_filter [_nbr_stages]._mem, y);

	// Averages both paths and outputs the result
	// Outputs the result
	y = _mm_mul_ps (y, _mm_set1_ps (0.5f));
	const auto     even = _mm_cvtss_f32 (_mm_shuffle_ps (y, y, 1));
	const auto     odd  = _mm_cvtss_f32 (y);
	const auto     low  = even + odd;

	return low;
}



/*
==============================================================================
Name: process_block
Description:
	Downsamples (x2) a block of samples.
	Input and output blocks may overlap, see assert() for details.
Input parameters:
	- in_ptr: Input array, containing nbr_spl * 2 samples.
	- nbr_spl: Number of samples to output, > 0
Output parameters:
	- out_ptr: Array for the output samples, capacity: nbr_spl samples.
Throws: Nothing
==============================================================================
*/

template <int NC>
void	Downsampler2xSseOld <NC>::process_block (float out_ptr [], const float in_ptr [], long nbr_spl) noexcept
{
	assert (in_ptr  != nullptr);
	assert (out_ptr != nullptr);
	assert (out_ptr <= in_ptr || out_ptr >= in_ptr + nbr_spl * 2);
	assert (nbr_spl > 0);

	const long     n4 = process_block_quad (
		out_ptr, nullptr, in_ptr, nbr_spl, store_low, bypass
	);

	for (long pos = n4; pos < nbr_spl; ++pos)
	{
		out_ptr [pos] = process_sample (in_ptr + pos * 2);
	}
}



/*
==============================================================================
Name: process_sample_split
Description:
	Split (spectrum-wise) in half a pair of samples. The lower part of the
	spectrum is a classic downsampling, equivalent to the output of
	process_sample().
	The higher part is the complementary signal: original filter response
	is flipped from left to right, becoming a high-pass filter with the same
	cutoff frequency. This signal is then critically sampled (decimation by 2),
	flipping the spectrum: Fs/4...Fs/2 becomes Fs/4...0.
Input parameters:
	- in_ptr: pointer on the pair of input samples.
Output parameters:
	- low: output sample, lower part of the spectrum (downsampling)
	- high: output sample, higher part of the spectrum.
Throws: Nothing
==============================================================================
*/

template <int NC>
void	Downsampler2xSseOld <NC>::process_sample_split (float &low, float &high, const float in_ptr [2]) noexcept
{
	assert (in_ptr != nullptr);

	// Combines two input samples and two mid-processing data
	const auto     spl_in  = _mm_loadu_ps (in_ptr);
	const auto     spl_mid = _mm_load_ps (_filter [_nbr_stages]._mem);
	constexpr auto shuf    = (2 << 0) | (3 << 2) | (0 << 4) | (1 << 6);
	auto           y       = _mm_shuffle_ps (spl_mid, spl_in, shuf);

	auto           mem     = _mm_load_ps (_filter [0]._mem);

	// Processes each stage
	StageProcSseV4 <_nbr_stages>::process_sample_pos (_filter.data (), y, mem);

	_mm_store_ps (_filter [_nbr_stages]._mem, y);

	// Outputs the result
	y = _mm_mul_ps (y, _mm_set1_ps (0.5f));
	const auto     even = _mm_cvtss_f32 (_mm_shuffle_ps (y, y, 1));
	const auto     odd  = _mm_cvtss_f32 (y);
	low  = even + odd;
	high = even - odd;
}



/*
==============================================================================
Name: process_block_split
Description:
	Split (spectrum-wise) in half a block of samples. The lower part of the
	spectrum is a classic downsampling, equivalent to the output of
	process_block().
	The higher part is the complementary signal: original filter response
	is flipped from left to right, becoming a high-pass filter with the same
	cutoff frequency. This signal is then critically sampled (decimation by 2),
	flipping the spectrum: Fs/4...Fs/2 becomes Fs/4...0.
	Input and output blocks may overlap, see assert() for details.
Input parameters:
	- in_ptr: Input array, containing nbr_spl * 2 samples.
	- nbr_spl: Number of samples for each output, > 0
Output parameters:
	- out_l_ptr: Array for the output samples, lower part of the spectrum
		(downsampling). Capacity: nbr_spl samples.
	- out_h_ptr: Array for the output samples, higher part of the spectrum.
		Capacity: nbr_spl samples.
Throws: Nothing
==============================================================================
*/

template <int NC>
void	Downsampler2xSseOld <NC>::process_block_split (float out_l_ptr [], float out_h_ptr [], const float in_ptr [], long nbr_spl) noexcept
{
	assert (in_ptr    != nullptr);
	assert (out_l_ptr != nullptr);
	assert (out_l_ptr <= in_ptr || out_l_ptr >= in_ptr + nbr_spl * 2);
	assert (out_h_ptr != nullptr);
	assert (out_h_ptr <= in_ptr || out_h_ptr >= in_ptr + nbr_spl * 2);
	assert (out_h_ptr != out_l_ptr);
	assert (nbr_spl > 0);

	const long     n4 = process_block_quad (
		out_l_ptr, out_h_ptr, in_ptr, nbr_spl, store_low, store_high
	);

	for (long pos = n4; pos < nbr_spl; ++pos)
	{
		process_sample_split (out_l_ptr [pos], out_h_ptr [pos], in_ptr + pos * 2);
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
void	Downsampler2xSseOld <NC>::clear_buffers () noexcept
{
	for (int i = 0; i < _nbr_stages + 1; ++i)
	{
		_mm_store_ps (_filter [i]._mem, _mm_setzero_ps ());
	}
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int NC>
constexpr int	Downsampler2xSseOld <NC>::_stage_width;
template <int NC>
constexpr int	Downsampler2xSseOld <NC>::_nbr_stages;
template <int NC>
constexpr int	Downsampler2xSseOld <NC>::_coef_shift;



template <int NC>
void	Downsampler2xSseOld <NC>::set_single_coef (int index, double coef) noexcept
{
	assert (index >= 0);
	assert (index < _nbr_stages * _stage_width);

	const int      stage = (index / _stage_width) + 1;
	const int      pos   = (index ^ _coef_shift) & (_stage_width - 1);
	_filter [stage]._coef [pos] = DataType (coef);
}



template <int NC>
template <typename FL, typename FH>
long	Downsampler2xSseOld <NC>::process_block_quad (float out_l_ptr [], float out_h_ptr [], const float in_ptr [], long nbr_spl, FL fnc_l, FH fnc_h) noexcept
{
	const auto     half   = _mm_set1_ps (0.5f);

	constexpr auto shuf_y = (2 << 0) | (3 << 2);
	constexpr auto shuf_0 = shuf_y | (0 << 4) | (1 << 6);
	constexpr auto shuf_1 = shuf_y | (2 << 4) | (3 << 6);

	const long     n4     = nbr_spl & ~(4-1);
	auto           y_3    = _mm_load_ps (_filter [_nbr_stages]._mem);
	for (long pos = 0; pos < n4; pos += 4)
	{
		const auto     x_0   = _mm_loadu_ps (in_ptr + pos * 2    );
		const auto     x_2   = _mm_loadu_ps (in_ptr + pos * 2 + 4);

		auto           y_0   = _mm_shuffle_ps (y_3, x_0, shuf_0);
		auto           mem_0 = _mm_load_ps (_filter [0]._mem);
		StageProcSseV4 <_nbr_stages>::process_sample_pos (_filter.data (), y_0, mem_0);
		_mm_store_ps (_filter [_nbr_stages]._mem, y_0);

		auto           y_1   = _mm_shuffle_ps (y_0, x_0, shuf_1);
		auto           mem_1 = _mm_load_ps (_filter [0]._mem);
		StageProcSseV4 <_nbr_stages>::process_sample_pos (_filter.data (), y_1, mem_1);
		_mm_store_ps (_filter [_nbr_stages]._mem, y_1);

		auto           y_2   = _mm_shuffle_ps (y_1, x_2, shuf_0);
		auto           mem_2 = _mm_load_ps (_filter [0]._mem);
		StageProcSseV4 <_nbr_stages>::process_sample_pos (_filter.data (), y_2, mem_2);
		_mm_store_ps (_filter [_nbr_stages]._mem, y_2);

		y_3 = _mm_shuffle_ps (y_2, x_2, shuf_1);
		auto           mem_3 = _mm_load_ps (_filter [0]._mem);
		StageProcSseV4 <_nbr_stages>::process_sample_pos (_filter.data (), y_3, mem_3);
		_mm_store_ps (_filter [_nbr_stages]._mem, y_3);

		const auto     u_01 = _mm_unpacklo_ps (y_0, y_1); // o0, o1, e0, e1
		const auto     u_23 = _mm_unpacklo_ps (y_2, y_3); // o2, o3, e2, e3
		const auto     odd  = _mm_movelh_ps (u_01, u_23);
		const auto     even = _mm_movehl_ps (u_23, u_01);
		fnc_l (out_l_ptr + pos, even, odd, half);
		fnc_h (out_h_ptr + pos, even, odd, half);
	}

	return n4;
}



template <int NC>
void	Downsampler2xSseOld <NC>::store_low (float *ptr, __m128 even, __m128 odd, __m128 half) noexcept
{
	const auto     low  = _mm_mul_ps (_mm_add_ps (even, odd), half);
	_mm_storeu_ps (ptr, low);
}



template <int NC>
void	Downsampler2xSseOld <NC>::store_high (float *ptr, __m128 even, __m128 odd, __m128 half) noexcept
{
	const auto     high = _mm_mul_ps (_mm_sub_ps (even, odd), half);
	_mm_storeu_ps (ptr, high);
}



}  // namespace hiir



#endif   // hiir_Downsampler2xSseOld_CODEHEADER_INCLUDED

#undef hiir_Downsampler2xSseOld_CURRENT_CODEHEADER



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
