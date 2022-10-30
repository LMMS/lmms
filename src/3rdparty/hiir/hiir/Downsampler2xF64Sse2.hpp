/*****************************************************************************

        Downsampler2xF64Sse2.hpp
        Author: Laurent de Soras, 2020

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#if ! defined (hiir_Downsampler2xF64Sse2_CODEHEADER_INCLUDED)
#define hiir_Downsampler2xF64Sse2_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/StageProcF64Sse2.h"

#include <cassert>



namespace hiir
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int NC>
constexpr int 	Downsampler2xF64Sse2 <NC>::_nbr_chn;
template <int NC>
constexpr int 	Downsampler2xF64Sse2 <NC>::NBR_COEFS;
template <int NC>
constexpr double	Downsampler2xF64Sse2 <NC>::_delay;



/*
==============================================================================
Name: ctor
Throws: Nothing
==============================================================================
*/

template <int NC>
Downsampler2xF64Sse2 <NC>::Downsampler2xF64Sse2 () noexcept
{
	for (int i = 0; i < _nbr_stages + 1; ++i)
	{
		_mm_store_pd (_filter [i]._coef, _mm_setzero_pd ());
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
void	Downsampler2xF64Sse2 <NC>::set_coefs (const double coef_arr []) noexcept
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
double	Downsampler2xF64Sse2 <NC>::process_sample (const double in_ptr [2]) noexcept
{
	assert (in_ptr != nullptr);

	auto           x  = _mm_loadu_pd (in_ptr);
	StageProcF64Sse2 <_nbr_stages>::process_sample_pos (x, _filter.data ());
	x = _mm_add_sd (x, _mm_shuffle_pd (x, x, 1));
	x = _mm_mul_sd (x, _mm_set_sd (0.5f));

	return _mm_cvtsd_f64 (x);
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
void	Downsampler2xF64Sse2 <NC>::process_block (double out_ptr [], const double in_ptr [], long nbr_spl) noexcept
{
	assert (in_ptr  != nullptr);
	assert (out_ptr != nullptr);
	assert (out_ptr <= in_ptr || out_ptr >= in_ptr + nbr_spl * 2);
	assert (nbr_spl > 0);

	const long     n2 = process_block_double (
		out_ptr, nullptr, in_ptr, nbr_spl, store_low, bypass
	);
	if (n2 < nbr_spl)
	{
		out_ptr [n2] = process_sample (in_ptr + n2 * 2);
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
void	Downsampler2xF64Sse2 <NC>::process_sample_split (double &low, double &high, const double in_ptr [2]) noexcept
{
	assert (in_ptr != nullptr);

	auto           x  = _mm_loadu_pd (in_ptr);
	StageProcF64Sse2 <_nbr_stages>::process_sample_pos (x, _filter.data ());
	x = _mm_mul_pd (x, _mm_set1_pd (0.5f));
	const auto     xr = _mm_shuffle_pd (x, x, 1);
	low  = _mm_cvtsd_f64 (_mm_add_sd (xr, x));
	high = _mm_cvtsd_f64 (_mm_sub_sd (xr, x));
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
void	Downsampler2xF64Sse2 <NC>::process_block_split (double out_l_ptr [], double out_h_ptr [], const double in_ptr [], long nbr_spl) noexcept
{
	assert (in_ptr    != nullptr);
	assert (out_l_ptr != nullptr);
	assert (out_l_ptr <= in_ptr || out_l_ptr >= in_ptr + nbr_spl * 2);
	assert (out_h_ptr != nullptr);
	assert (out_h_ptr <= in_ptr || out_h_ptr >= in_ptr + nbr_spl * 2);
	assert (out_h_ptr != out_l_ptr);
	assert (nbr_spl > 0);

	const long     n2 = process_block_double (
		out_l_ptr, out_h_ptr, in_ptr, nbr_spl, store_low, store_high
	);
	if (n2 < nbr_spl)
	{
		process_sample_split (out_l_ptr [n2], out_h_ptr [n2], in_ptr + n2 * 2);
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
void	Downsampler2xF64Sse2 <NC>::clear_buffers () noexcept
{
	for (int i = 0; i < _nbr_stages + 1; ++i)
	{
		_mm_store_pd (_filter [i]._mem, _mm_setzero_pd ());
	}
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int NC>
constexpr int	Downsampler2xF64Sse2 <NC>::_stage_width;
template <int NC>
constexpr int	Downsampler2xF64Sse2 <NC>::_nbr_stages;



template <int NC>
template <typename FL, typename FH>
long	Downsampler2xF64Sse2 <NC>::process_block_double (double out_l_ptr [], double out_h_ptr [], const double in_ptr [], long nbr_spl, FL fnc_l, FH fnc_h) noexcept
{
	const auto     half = _mm_set1_pd (0.5f);

	const long     n2   = nbr_spl & ~(2-1);
	for (long pos = 0; pos < n2; pos += 2)
	{
		auto           x_0 = _mm_loadu_pd (in_ptr + pos * 2);
		auto           x_1 = _mm_loadu_pd (in_ptr + pos * 2 + 2);
		StageProcF64Sse2 <_nbr_stages>::process_sample_pos (x_0, _filter.data ());
		StageProcF64Sse2 <_nbr_stages>::process_sample_pos (x_1, _filter.data ());
		const auto     odd  = _mm_unpacklo_pd (x_0, x_1);
		const auto     even = _mm_unpackhi_pd (x_0, x_1);
		fnc_l (out_l_ptr + pos, even, odd, half);
		fnc_h (out_h_ptr + pos, even, odd, half);
	}

	return n2;
}



template <int NC>
void	Downsampler2xF64Sse2 <NC>::store_low (double *ptr, __m128d even, __m128d odd, __m128d half) noexcept
{
	const auto     low  = _mm_mul_pd (_mm_add_pd (even, odd), half);
	_mm_storeu_pd (ptr, low);
}



template <int NC>
void	Downsampler2xF64Sse2 <NC>::store_high (double *ptr, __m128d even, __m128d odd, __m128d half) noexcept
{
	const auto     high = _mm_mul_pd (_mm_sub_pd (even, odd), half);
	_mm_storeu_pd (ptr, high);
}



}  // namespace hiir



#endif   // hiir_Downsampler2xF64Sse2_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
