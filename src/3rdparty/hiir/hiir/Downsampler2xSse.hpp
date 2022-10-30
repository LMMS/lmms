/*****************************************************************************

        Downsampler2xSse.hpp
        Author: Laurent de Soras, 2020

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#if ! defined (hiir_Downsampler2xSse_CODEHEADER_INCLUDED)
#define hiir_Downsampler2xSse_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/StageProcSseV2.h"

#include <cassert>



namespace hiir
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int NC>
constexpr int 	Downsampler2xSse <NC>::_nbr_chn;
template <int NC>
constexpr int 	Downsampler2xSse <NC>::NBR_COEFS;
template <int NC>
constexpr double	Downsampler2xSse <NC>::_delay;



/*
==============================================================================
Name: ctor
Throws: Nothing
==============================================================================
*/

template <int NC>
Downsampler2xSse <NC>::Downsampler2xSse () noexcept
{
	for (int i = 0; i < _nbr_stages + 1; ++i)
	{
		_mm_store_ps (_filter [i]._coef, _mm_setzero_ps ());
	}
	if (NBR_COEFS < _nbr_stages * 2)
	{
		_filter [_nbr_stages]._coef [0] = 1;
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
void	Downsampler2xSse <NC>::set_coefs (const double coef_arr []) noexcept
{
	assert (coef_arr != nullptr);

	for (int i = 0; i < NBR_COEFS; ++i)
	{
		const int      stage = (i / _stage_width) + 1;
		const int      pos   = (i ^ 1) & (_stage_width - 1);
		_filter [stage]._coef [pos] = DataType (coef_arr [i]);
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
float	Downsampler2xSse <NC>::process_sample (const float in_ptr [2]) noexcept
{
	assert (in_ptr != nullptr);

	auto           x = _mm_loadl_pi (
		_mm_setzero_ps (), reinterpret_cast <const __m64 *> (in_ptr)
	);
	StageProcSseV2 <_nbr_stages>::process_sample_pos (x, _filter.data ());
	x = _mm_add_ss (x, _mm_shuffle_ps (x, x, 1));
	x = _mm_mul_ss (x, _mm_set_ss (0.5f));

	return _mm_cvtss_f32 (x);
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
void	Downsampler2xSse <NC>::process_block (float out_ptr [], const float in_ptr [], long nbr_spl) noexcept
{
	assert (in_ptr  != nullptr);
	assert (out_ptr != nullptr);
	assert (out_ptr <= in_ptr || out_ptr >= in_ptr + nbr_spl * 2);
	assert (nbr_spl > 0);

#if ! defined (_MSC_VER)
	// This version is slower than copy-pasta when compiled with MSVC
	const long     n4   = process_block_quad (
		out_ptr, nullptr, in_ptr, nbr_spl, store_low, bypass
	);

	const auto     half = _mm_set1_ps (0.5f);
#else
	const auto     half = _mm_set1_ps (0.5f);

	const long     n4 = nbr_spl & ~(4-1);
	for (long pos = 0; pos < n4; pos += 4)
	{
		auto           x_0 = _mm_loadu_ps (in_ptr + pos * 2);
		auto           x_1 = _mm_movehl_ps (x_0, x_0);
		auto           x_2 = _mm_loadu_ps (in_ptr + pos * 2 + 4);
		auto           x_3 = _mm_movehl_ps (x_2, x_2);
		StageProcSseV2 <_nbr_stages>::process_sample_pos (x_0, _filter.data ());
		StageProcSseV2 <_nbr_stages>::process_sample_pos (x_1, _filter.data ());
		StageProcSseV2 <_nbr_stages>::process_sample_pos (x_2, _filter.data ());
		StageProcSseV2 <_nbr_stages>::process_sample_pos (x_3, _filter.data ());
		const auto     u_01 = _mm_unpacklo_ps (x_0, x_1); // o0, o1, e0, e1
		const auto     u_23 = _mm_unpacklo_ps (x_2, x_3); // o2, o3, e2, e3
		const auto     odd  = _mm_movelh_ps (u_01, u_23);
		const auto     even = _mm_movehl_ps (u_23, u_01);
		const auto     y    = _mm_mul_ps (_mm_add_ps (even, odd), half);
		_mm_storeu_ps (out_ptr + pos, y);
	}

#endif
	for (long pos = n4; pos < nbr_spl; ++pos)
	{
		auto           x  = _mm_loadl_pi (
			half, reinterpret_cast <const __m64 *> (in_ptr + pos * 2)
		);
		StageProcSseV2 <_nbr_stages>::process_sample_pos (x, _filter.data ());
		x = _mm_add_ss (x, _mm_shuffle_ps (x, x, 1));
		x = _mm_mul_ss (x, half);
		out_ptr [pos] = _mm_cvtss_f32 (x);
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
	- in_ptr: pointer on the pair of input samples
Output parameters:
	- low: output sample, lower part of the spectrum (downsampling)
	- high: output sample, higher part of the spectrum.
Throws: Nothing
==============================================================================
*/

template <int NC>
void	Downsampler2xSse <NC>::process_sample_split (float &low, float &high, const float in_ptr [2]) noexcept
{
	assert (in_ptr != nullptr);

	auto           x = _mm_loadl_pi (
		_mm_setzero_ps (), reinterpret_cast <const __m64 *> (in_ptr)
	);
	StageProcSseV2 <_nbr_stages>::process_sample_pos (x, _filter.data ());
	x = _mm_mul_ps (x, _mm_set1_ps (0.5f));
	const auto     xr = _mm_shuffle_ps (x, x, 1);
	low  = _mm_cvtss_f32 (_mm_add_ss (xr, x));
	high = _mm_cvtss_f32 (_mm_sub_ss (xr, x));
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
 		Make sure that the 8 bytes following the end of the block are valid
		readable memory locations
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
void	Downsampler2xSse <NC>::process_block_split (float out_l_ptr [], float out_h_ptr [], const float in_ptr [], long nbr_spl) noexcept
{
	assert (in_ptr    != nullptr);
	assert (out_l_ptr != nullptr);
	assert (out_l_ptr <= in_ptr || out_l_ptr >= in_ptr + nbr_spl * 2);
	assert (out_h_ptr != nullptr);
	assert (out_h_ptr <= in_ptr || out_h_ptr >= in_ptr + nbr_spl * 2);
	assert (out_h_ptr != out_l_ptr);
	assert (nbr_spl > 0);

#if ! defined (_MSC_VER)
	// This version is slower than copy-pasta when compiled with MSVC
	const long     n4   = process_block_quad (
		out_l_ptr, out_h_ptr, in_ptr, nbr_spl, store_low, store_high
	);

	const auto     half = _mm_set1_ps (0.5f);
#else
	const auto     half = _mm_set1_ps (0.5f);

	const long     n4 = nbr_spl & ~(4-1);
	for (long pos = 0; pos < n4; pos += 4)
	{
		auto           x_0 = _mm_loadu_ps (in_ptr + pos * 2);
		auto           x_1 = _mm_movehl_ps (x_0, x_0);
		auto           x_2 = _mm_loadu_ps (in_ptr + pos * 2 + 4);
		auto           x_3 = _mm_movehl_ps (x_2, x_2);
		StageProcSseV2 <_nbr_stages>::process_sample_pos (x_0, _filter.data ());
		StageProcSseV2 <_nbr_stages>::process_sample_pos (x_1, _filter.data ());
		StageProcSseV2 <_nbr_stages>::process_sample_pos (x_2, _filter.data ());
		StageProcSseV2 <_nbr_stages>::process_sample_pos (x_3, _filter.data ());
		const auto     u_01 = _mm_unpacklo_ps (x_0, x_1); // o0, o1, e0, e1
		const auto     u_23 = _mm_unpacklo_ps (x_2, x_3); // o2, o3, e2, e3
		const auto     odd  = _mm_movelh_ps (u_01, u_23);
		const auto     even = _mm_movehl_ps (u_23, u_01);
		const auto     low  = _mm_mul_ps (_mm_add_ps (even, odd), half);
		const auto     high = _mm_mul_ps (_mm_sub_ps (even, odd), half);
		_mm_storeu_ps (out_l_ptr + pos, low );
		_mm_storeu_ps (out_h_ptr + pos, high);
	}

#endif
	for (long pos = n4; pos < nbr_spl; ++pos)
	{
		auto           x  = _mm_loadl_pi (
			half, reinterpret_cast <const __m64 *> (in_ptr + pos * 2)
		);
		StageProcSseV2 <_nbr_stages>::process_sample_pos (x, _filter.data ());
		x = _mm_mul_ps (x, half);
		const auto     xr = _mm_shuffle_ps (x, x, 1);
		out_l_ptr [pos] = _mm_cvtss_f32 (_mm_add_ss (xr, x));
		out_h_ptr [pos] = _mm_cvtss_f32 (_mm_sub_ss (xr, x));
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
void	Downsampler2xSse <NC>::clear_buffers () noexcept
{
	for (int i = 0; i < _nbr_stages + 1; ++i)
	{
		_mm_store_ps (_filter [i]._mem, _mm_setzero_ps ());
	}
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int NC>
constexpr int	Downsampler2xSse <NC>::_stage_width;
template <int NC>
constexpr int	Downsampler2xSse <NC>::_nbr_stages;



template <int NC>
template <typename FL, typename FH>
long	Downsampler2xSse <NC>::process_block_quad (float out_l_ptr [], float out_h_ptr [], const float in_ptr [], long nbr_spl, FL fnc_l, FH fnc_h) noexcept
{
	const auto     half = _mm_set1_ps (0.5f);

	const long     n4   = nbr_spl & ~(4-1);
	for (long pos = 0; pos < n4; pos += 4)
	{
		auto           x_0 = _mm_loadu_ps (in_ptr + pos * 2);
		auto           x_1 = _mm_movehl_ps (x_0, x_0);
		auto           x_2 = _mm_loadu_ps (in_ptr + pos * 2 + 4);
		auto           x_3 = _mm_movehl_ps (x_2, x_2);
		StageProcSseV2 <_nbr_stages>::process_sample_pos (x_0, _filter.data ());
		StageProcSseV2 <_nbr_stages>::process_sample_pos (x_1, _filter.data ());
		StageProcSseV2 <_nbr_stages>::process_sample_pos (x_2, _filter.data ());
		StageProcSseV2 <_nbr_stages>::process_sample_pos (x_3, _filter.data ());
		const auto     u_01 = _mm_unpacklo_ps (x_0, x_1); // o0, o1, e0, e1
		const auto     u_23 = _mm_unpacklo_ps (x_2, x_3); // o2, o3, e2, e3
		const auto     odd  = _mm_movelh_ps (u_01, u_23);
		const auto     even = _mm_movehl_ps (u_23, u_01);
		fnc_l (out_l_ptr + pos, even, odd, half);
		fnc_h (out_h_ptr + pos, even, odd, half);
	}

	return n4;
}



template <int NC>
void	Downsampler2xSse <NC>::store_low (float *ptr, __m128 even, __m128 odd, __m128 half) noexcept
{
	const auto     low  = _mm_mul_ps (_mm_add_ps (even, odd), half);
	_mm_storeu_ps (ptr, low);
}



template <int NC>
void	Downsampler2xSse <NC>::store_high (float *ptr, __m128 even, __m128 odd, __m128 half) noexcept
{
	const auto     high = _mm_mul_ps (_mm_sub_ps (even, odd), half);
	_mm_storeu_ps (ptr, high);
}



}  // namespace hiir



#endif   // hiir_Downsampler2xSse_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
