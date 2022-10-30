/*****************************************************************************

        Downsampler2x4Sse.hpp
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (hiir_Downsampler2x4Sse_CODEHEADER_INCLUDED)
#define hiir_Downsampler2x4Sse_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/StageProc4Sse.h"

#include <cassert>



namespace hiir
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int NC>
constexpr int 	Downsampler2x4Sse <NC>::_nbr_chn;
template <int NC>
constexpr int 	Downsampler2x4Sse <NC>::NBR_COEFS;
template <int NC>
constexpr double	Downsampler2x4Sse <NC>::_delay;



/*
==============================================================================
Name: ctor
Throws: Nothing
==============================================================================
*/

template <int NC>
Downsampler2x4Sse <NC>::Downsampler2x4Sse () noexcept
:	_filter ()
{
	for (int i = 0; i < NBR_COEFS + 2; ++i)
	{
		_mm_store_ps (_filter [i]._coef, _mm_setzero_ps ());
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
void	Downsampler2x4Sse <NC>::set_coefs (const double coef_arr []) noexcept
{
	assert (coef_arr != nullptr);

	for (int i = 0; i < NBR_COEFS; ++i)
	{
		_mm_store_ps (
			_filter [i + 2]._coef, _mm_set1_ps (DataType (coef_arr [i]))
		);
	}
}



/*
==============================================================================
Name: process_sample
Description:
	Downsamples (x2) one pair of vector of 4 samples, to generate one output
	vector.
Input parameters:
	- in_ptr: pointer on the two vectors to decimate. No alignment constraint.
Returns: Samplerate-reduced vector.
Throws: Nothing
==============================================================================
*/

template <int NC>
__m128	Downsampler2x4Sse <NC>::process_sample (const float in_ptr [_nbr_chn * 2]) noexcept
{
	assert (in_ptr != nullptr);

	const __m128   in_0 = _mm_loadu_ps (in_ptr           );
	const __m128   in_1 = _mm_loadu_ps (in_ptr + _nbr_chn);

	return process_sample (in_0, in_1);
}



/*
==============================================================================
Name: process_sample
Description:
	Downsamples (x2) one pair of vector of 4 samples, to generate one output
	vector.
Input parameters:
	- in_0: vector at t
	- in_1: vector at t + 1
Returns: Samplerate-reduced vector.
Throws: Nothing
==============================================================================
*/

template <int NC>
__m128	Downsampler2x4Sse <NC>::process_sample (__m128 in_0, __m128 in_1) noexcept
{
	__m128         spl_0 = in_1;
	__m128         spl_1 = in_0;

	StageProc4Sse <NBR_COEFS>::process_sample_pos (
		NBR_COEFS, spl_0, spl_1, &_filter [0]
	);

	const __m128   sum = _mm_add_ps (spl_0, spl_1);
	const __m128   out = _mm_mul_ps (sum, _mm_set1_ps (0.5f));

	return out;
}



/*
==============================================================================
Name: process_block
Description:
	Downsamples (x2) a block of vectors of 4 samples.
	Input and output blocks may overlap, see assert() for details.
Input parameters:
	- in_ptr: Input array, containing nbr_spl * 2 vectors.
		No alignment constraint.
	- nbr_spl: Number of vectors to output, > 0
Output parameters:
	- out_ptr: Array for the output vectors, capacity: nbr_spl vectors.
		No alignment constraint.
Throws: Nothing
==============================================================================
*/

template <int NC>
void	Downsampler2x4Sse <NC>::process_block (float out_ptr [], const float in_ptr [], long nbr_spl) noexcept
{
	assert (in_ptr  != nullptr);
	assert (out_ptr != nullptr);
	assert (out_ptr <= in_ptr || out_ptr >= in_ptr + nbr_spl * (_nbr_chn * 2));
	assert (nbr_spl > 0);

	long           pos = 0;
	do
	{
		const __m128   val = process_sample (in_ptr + pos * (_nbr_chn * 2));
		_mm_storeu_ps (out_ptr + pos * _nbr_chn, val);
		++ pos;
	}
	while (pos < nbr_spl);
}



/*
==============================================================================
Name: process_sample_split
Description:
	Split (spectrum-wise) in half a pair of vector of 4 samples. The lower part
	of the spectrum is a classic downsampling, equivalent to the output of
	process_sample().
	The higher part is the complementary signal: original filter response
	is flipped from left to right, becoming a high-pass filter with the same
	cutoff frequency. This signal is then critically sampled (decimation by 2),
	flipping the spectrum: Fs/4...Fs/2 becomes Fs/4...0.
Input parameters:
	- in_ptr: pointer on the pair of input vectors. No alignment constraint.
Output parameters:
	- low: output vector, lower part of the spectrum (downsampling)
	- high: output vector, higher part of the spectrum.
Throws: Nothing
==============================================================================
*/

template <int NC>
void	Downsampler2x4Sse <NC>::process_sample_split (__m128 &low, __m128 &high, const float in_ptr [_nbr_chn * 2]) noexcept
{
	assert (in_ptr != nullptr);

	const __m128   in_0 = _mm_loadu_ps (in_ptr           );
	const __m128   in_1 = _mm_loadu_ps (in_ptr + _nbr_chn);

	process_sample_split (low, high, in_0, in_1);
}



/*
==============================================================================
Name: process_sample_split
Description:
	Split (spectrum-wise) in half a pair of vector of 4 samples. The lower part
	of the spectrum is a classic downsampling, equivalent to the output of
	process_sample().
	The higher part is the complementary signal: original filter response
	is flipped from left to right, becoming a high-pass filter with the same
	cutoff frequency. This signal is then critically sampled (decimation by 2),
	flipping the spectrum: Fs/4...Fs/2 becomes Fs/4...0.
Input parameters:
	- in_0: vector at t
	- in_1: vector at t + 1
Output parameters:
	- low: output vector, lower part of the spectrum (downsampling)
	- high: output vector, higher part of the spectrum.
Throws: Nothing
==============================================================================
*/

template <int NC>
void	Downsampler2x4Sse <NC>::process_sample_split (__m128 &low, __m128 &high, __m128 in_0, __m128 in_1) noexcept
{
	__m128         spl_0 = in_1;
	__m128         spl_1 = in_0;

	StageProc4Sse <NBR_COEFS>::process_sample_pos (
		NBR_COEFS, spl_0, spl_1, &_filter [0]
	);

	const __m128   sum = _mm_add_ps (spl_0, spl_1);
	low  = _mm_mul_ps (sum, _mm_set1_ps (0.5f));
	high = _mm_sub_ps (spl_0, low);
}



/*
==============================================================================
Name: process_block_split
Description:
	Split (spectrum-wise) in half a pair of vector of 4 samples. The lower part
	of the spectrum is a classic downsampling, equivalent to the output of
	process_block().
	The higher part is the complementary signal: original filter response
	is flipped from left to right, becoming a high-pass filter with the same
	cutoff frequency. This signal is then critically sampled (decimation by 2),
	flipping the spectrum: Fs/4...Fs/2 becomes Fs/4...0.
	Input and output blocks may overlap, see assert() for details.
Input parameters:
	- in_ptr: Input array, containing nbr_spl * 2 vectors.
		No alignment constraint.
	- nbr_spl: Number of vectors for each output, > 0
Output parameters:
	- out_l_ptr: Array for the output vectors, lower part of the spectrum
		(downsampling). Capacity: nbr_spl vectors.
		No alignment constraint.
	- out_h_ptr: Array for the output vectors, higher part of the spectrum.
		Capacity: nbr_spl vectors.
		No alignment constraint.
Throws: Nothing
==============================================================================
*/

template <int NC>
void	Downsampler2x4Sse <NC>::process_block_split (float out_l_ptr [], float out_h_ptr [], const float in_ptr [], long nbr_spl) noexcept
{
	assert (in_ptr    != nullptr);
	assert (out_l_ptr != nullptr);
	assert (out_l_ptr <= in_ptr || out_l_ptr >= in_ptr + nbr_spl * (_nbr_chn * 2));
	assert (out_h_ptr != nullptr);
	assert (out_h_ptr <= in_ptr || out_h_ptr >= in_ptr + nbr_spl * (_nbr_chn * 2));
	assert (out_h_ptr != out_l_ptr);
	assert (nbr_spl > 0);

	long           pos = 0;
	do
	{
		__m128         low;
		__m128         high;
		process_sample_split (low, high, in_ptr + pos * (_nbr_chn * 2));
		_mm_storeu_ps (out_l_ptr + pos * _nbr_chn, low);
		_mm_storeu_ps (out_h_ptr + pos * _nbr_chn, high);
		++ pos;
	}
	while (pos < nbr_spl);
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
void	Downsampler2x4Sse <NC>::clear_buffers () noexcept
{
	for (int i = 0; i < NBR_COEFS + 2; ++i)
	{
		_mm_store_ps (_filter [i]._mem, _mm_setzero_ps ());
	}
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}  // namespace hiir



#endif   // hiir_Downsampler2x4Sse_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
