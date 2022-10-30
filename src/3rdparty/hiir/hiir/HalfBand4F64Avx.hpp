/*****************************************************************************

        HalfBand4F64Avx.hpp
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#if ! defined (hiir_HalfBand4F64Avx_CODEHEADER_INCLUDED)
#define hiir_HalfBand4F64Avx_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/StageProc4F64Avx.h"

#include <cassert>



namespace hiir
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int NC>
constexpr int 	HalfBand4F64Avx <NC>::_nbr_chn;
template <int NC>
constexpr int 	HalfBand4F64Avx <NC>::NBR_COEFS;
template <int NC>
constexpr double	HalfBand4F64Avx <NC>::_delay;



/*
==============================================================================
Name: ctor
Throws: Nothing
==============================================================================
*/

template <int NC>
HalfBand4F64Avx <NC>::HalfBand4F64Avx () noexcept
:	_bifilter ()
,	_phase (0)
{
	for (int i = 0; i < NBR_COEFS + 2; ++i)
	{
		_mm256_store_pd (_bifilter [0] [i]._coef, _mm256_setzero_pd ());
		_mm256_store_pd (_bifilter [1] [i]._coef, _mm256_setzero_pd ());
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
void	HalfBand4F64Avx <NC>::set_coefs (const double coef_arr []) noexcept
{
	assert (coef_arr != nullptr);

	for (int i = 0; i < NBR_COEFS; ++i)
	{
		const auto     c = _mm256_set1_pd (DataType (coef_arr [i]));
		_mm256_store_pd (_bifilter [0] [i + 2]._coef, c);
		_mm256_store_pd (_bifilter [1] [i + 2]._coef, c);
	}
}



/*
==============================================================================
Name: process_sample
Description:
	Performs a half-band low-pass filtering on a multichannel vector.
Input parameters:
	- in: sample from the stream to be filtered.
Returns: filtered vector.
Throws: Nothing
==============================================================================
*/

template <int NC>
__m256d	HalfBand4F64Avx <NC>::process_sample (__m256d input) noexcept
{
	const auto     path_arr = process_2_paths (input);
	const auto     low      = _mm256_add_pd (path_arr._e, path_arr._o);

	return low;
}



/*
==============================================================================
Name: process_block
Description:
	Performs a half-band low-pass filtering on a block of multichannel vectors.
	Input and output blocks may overlap, see assert() for details.
Input parameters:
	- in_ptr: Input array, containing nbr_spl vectors.
	- nbr_spl: Number of vectors to output, > 0
Output parameters:
	- out_ptr: Array for the output vectors, capacity: nbr_spl vectors.
Throws: Nothing
==============================================================================
*/

template <int NC>
void	HalfBand4F64Avx <NC>::process_block (double out_ptr [], const double in_ptr [], long nbr_spl) noexcept
{
	assert (out_ptr != nullptr);
	assert (out_ptr <= in_ptr || out_ptr >= in_ptr + nbr_spl * _nbr_chn);

	process_block_2_paths (
		out_ptr, nullptr, in_ptr, nbr_spl, store_low, bypass
	);
}



/*
==============================================================================
Name: process_sample_hpf
Description:
	Performs a half-band high-pass filtering on a multichannel vector.
Input parameters:
	- in: sample from the stream to be filtered.
Returns: filtered vector.
Throws: Nothing
==============================================================================
*/

template <int NC>
__m256d	HalfBand4F64Avx <NC>::process_sample_hpf (__m256d input) noexcept
{
	const auto     path_arr = process_2_paths (input);
	const auto     high     = _mm256_sub_pd (path_arr._e, path_arr._o);

	return high;
}



/*
==============================================================================
Name: process_block_hpf
Description:
	Performs a half-band high-pass filtering on a block of multichannel
	vectors.
	Input and output blocks may overlap, see assert() for details.
Input parameters:
	- in_ptr: Input array, containing nbr_spl vectors.
	- nbr_spl: Number of vectors to output, > 0
Output parameters:
	- out_ptr: Array for the output vectors, capacity: nbr_spl vectors.
Throws: Nothing
==============================================================================
*/

template <int NC>
void	HalfBand4F64Avx <NC>::process_block_hpf (double out_ptr [], const double in_ptr [], long nbr_spl) noexcept
{
	assert (out_ptr != nullptr);
	assert (out_ptr <= in_ptr || out_ptr >= in_ptr + nbr_spl * _nbr_chn);

	process_block_2_paths (
		nullptr, out_ptr, in_ptr, nbr_spl, bypass, store_high
	);
}



/*
==============================================================================
Name: process_sample_split
Description:
	Splits (spectrum-wise) in half a vector from a 4-channel stream. Both part
	are results of a low-pass and a high-pass filtering, equivalent to the
	output 	of process_sample() and process_sample_hpf().
Input parameters:
	- input: sample from the stream to be filtered.
Output parameters:
	- low: output sample, lower part of the spectrum.
	- high: output sample, higher part of the spectrum.
Throws: Nothing
==============================================================================
*/

template <int NC>
void	HalfBand4F64Avx <NC>::process_sample_split (__m256d &low, __m256d &high, __m256d input) noexcept
{
	const auto     path_arr = process_2_paths (input);
	low  = _mm256_add_pd (path_arr._e, path_arr._o);
	high = _mm256_sub_pd (path_arr._e, path_arr._o);
}



/*
==============================================================================
Name: process_block_split
Description:
	Splits (spectrum-wise) in half a block of 4-channel vectors. Both part are
	results of a low-pass and a high-pass filtering, equivalent to the output
	of process_sample() and process_sample_hpf().
	Input and output blocks may overlap, see assert() for details.
Input parameters:
	- in_ptr: Input array, containing nbr_spl vectors.
	- nbr_spl: Number of vectors to output, > 0
Output parameters:
	- out_l_ptr: Array for the output samples, lower part of the spectrum
		Capacity: nbr_spl vectors.
	- out_h_ptr: Array for the output samples, higher part of the spectrum.
		Capacity: nbr_spl vectors.
Throws: Nothing
==============================================================================
*/

template <int NC>
void	HalfBand4F64Avx <NC>::process_block_split (double out_l_ptr [], double out_h_ptr [], const double in_ptr [], long nbr_spl) noexcept
{
	assert (out_l_ptr != nullptr);
	assert (out_h_ptr != nullptr);
	assert (out_l_ptr <= in_ptr || out_l_ptr >= in_ptr + nbr_spl * _nbr_chn);
	assert (out_h_ptr <= in_ptr || out_h_ptr >= in_ptr + nbr_spl * _nbr_chn);
	assert (   out_l_ptr + nbr_spl * _nbr_chn <= out_h_ptr
	        || out_h_ptr + nbr_spl * _nbr_chn <= out_l_ptr);

	process_block_2_paths (
		out_l_ptr, out_h_ptr, in_ptr, nbr_spl, store_low, store_high
	);
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
void	HalfBand4F64Avx <NC>::clear_buffers () noexcept
{
	_phase = 0;
	for (int i = 0; i < NBR_COEFS + 2; ++i)
	{
		_mm256_store_pd (_bifilter [0] [i]._mem, _mm256_setzero_pd ());
		_mm256_store_pd (_bifilter [1] [i]._mem, _mm256_setzero_pd ());
	}
	_mm256_store_pd (_prev, _mm256_setzero_pd ());
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int NC>
constexpr int	HalfBand4F64Avx <NC>::_nbr_phases;



// Shared processing function, outputs both paths of the all-pass filter pair.
// Returns: even path, odd path
template <int NC>
typename HalfBand4F64Avx <NC>::EvenOdd	HalfBand4F64Avx <NC>::process_2_paths (__m256d input) noexcept
{
	const auto     half = _mm256_set1_pd (0.5f);
	input = _mm256_mul_pd (input, half);

	auto           path_arr = EvenOdd { input, _mm256_load_pd (_prev) };

	StageProc4F64Avx <NBR_COEFS>::process_sample_pos (
		NBR_COEFS, path_arr._e, path_arr._o, &_bifilter [_phase] [0]
	);

	_mm256_store_pd (_prev, input);
	_phase = 1 - _phase;

	return path_arr;
}



// One of the out_l_ptr or out_h_ptr can be nullptr
template <int NC>
template <typename FL, typename FH>
void	HalfBand4F64Avx <NC>::process_block_2_paths (double out_l_ptr [], double out_h_ptr [], const double in_ptr [], long nbr_spl, FL fnc_l, FH fnc_h) noexcept
{
	assert (in_ptr != nullptr);
	assert (nbr_spl > 0);

	long           pos  = 0;
	const auto     half = _mm256_set1_pd (0.5f);

	if (_phase == 1)
	{
		const auto     path_arr = process_2_paths (_mm256_loadu_pd (in_ptr));
		fnc_l (out_l_ptr, path_arr._e, path_arr._o);
		fnc_h (out_h_ptr, path_arr._e, path_arr._o);
		++ pos;
	}

	const long     end  = ((nbr_spl - pos) & -_nbr_phases) + pos;
	auto           prev = _mm256_load_pd (_prev);
	while (pos < end)
	{
		const auto     ofs_0   = pos * _nbr_chn;
		const auto     ofs_1   = ofs_0 + _nbr_chn;
		auto           input_0 = _mm256_loadu_pd (in_ptr + ofs_0);
		auto           input_1 = _mm256_loadu_pd (in_ptr + ofs_1);

		input_0 = _mm256_mul_pd (input_0, half);
		input_1 = _mm256_mul_pd (input_1, half);

		auto           tmp_0 = input_0;
		auto           tmp_1 = prev;
		StageProc4F64Avx <NBR_COEFS>::process_sample_pos (
			NBR_COEFS, tmp_0, tmp_1, &_bifilter [0] [0]
		);

		auto           tmp_2   = input_1;
		auto           tmp_3   = input_0;  // prev
		StageProc4F64Avx <NBR_COEFS>::process_sample_pos (
			NBR_COEFS, tmp_2, tmp_3, &_bifilter [1] [0]
		);

		fnc_l (out_l_ptr + ofs_0, tmp_0, tmp_1);
		fnc_h (out_h_ptr + ofs_0, tmp_0, tmp_1);
		fnc_l (out_l_ptr + ofs_1, tmp_2, tmp_3);
		fnc_h (out_h_ptr + ofs_1, tmp_2, tmp_3);

		prev = input_1;
		pos += 2;
	}
	_mm256_store_pd (_prev, prev);

	if (pos < nbr_spl)
	{
		assert (pos + 1 == nbr_spl);
		const auto     ofs      = pos * _nbr_chn;
		const auto     path_arr = process_2_paths (_mm256_loadu_pd (in_ptr + ofs));
		fnc_l (out_l_ptr + ofs, path_arr._e, path_arr._o);
		fnc_h (out_h_ptr + ofs, path_arr._e, path_arr._o);
	}
}



template <int NC>
void	HalfBand4F64Avx <NC>::store_low (double *ptr, __m256d tmp_0, __m256d tmp_1) noexcept
{
	const auto     low = _mm256_add_pd (tmp_0, tmp_1);
	_mm256_storeu_pd (ptr, low);
}



template <int NC>
void	HalfBand4F64Avx <NC>::store_high (double *ptr, __m256d tmp_0, __m256d tmp_1) noexcept
{
	const auto     high = _mm256_sub_pd (tmp_0, tmp_1);
	_mm256_storeu_pd (ptr, high);
}



}  // namespace hiir



#endif   // hiir_HalfBand4F64Avx_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

