/*****************************************************************************

        HalfBandF64Sse2.hpp
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (hiir_HalfBandF64Sse2_CODEHEADER_INCLUDED)
#define hiir_HalfBandF64Sse2_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/StageProcSseV4.h"

#include <mmintrin.h>

#include <cassert>



namespace hiir
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int NC>
constexpr int 	HalfBandF64Sse2 <NC>::_nbr_chn;
template <int NC>
constexpr int 	HalfBandF64Sse2 <NC>::NBR_COEFS;
template <int NC>
constexpr double	HalfBandF64Sse2 <NC>::_delay;



/*
==============================================================================
Name: ctor
Throws: Nothing
==============================================================================
*/

template <int NC>
HalfBandF64Sse2 <NC>::HalfBandF64Sse2 () noexcept
:	_bifilter ()
,	_prev (0)
,	_phase (0)
{
	for (int phase = 0; phase < _nbr_phases; ++phase)
	{
		for (int i = 0; i < _nbr_stages + 1; ++i)
		{
		   _mm_store_pd (_bifilter [phase] [i]._coef, _mm_setzero_pd ());
		}
		if (NBR_COEFS < _nbr_stages * 2)
		{
			_bifilter [phase] [_nbr_stages]._coef [0] = 1;
		}
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
void	HalfBandF64Sse2 <NC>::set_coefs (const double coef_arr []) noexcept
{
	assert (coef_arr != nullptr);

   for (int phase = 0; phase < _nbr_phases; ++phase)
   {
	   for (int i = 0; i < NBR_COEFS; ++i)
	   {
		   const int      stage = (i / _stage_width) + 1;
		   const int      pos   = (i ^ 1) & (_stage_width - 1);
		   _bifilter [phase] [stage]._coef [pos] = DataType (coef_arr [i]);
	   }
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
double	HalfBandF64Sse2 <NC>::process_sample (double input) noexcept
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
void	HalfBandF64Sse2 <NC>::process_block (double out_ptr [], const double in_ptr [], long nbr_spl) noexcept
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
	Performs a half-band high-pass filtering on a sample.
Input parameters:
	- in: sample from the stream to be filtered.
Returns: filtered sample.
Throws: Nothing
==============================================================================
*/

template <int NC>
double	HalfBandF64Sse2 <NC>::process_sample_hpf (double input) noexcept
{
	const auto     y    = process_2_paths (input);
	const auto     high = y [0] - y [1];

	return high;
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
void	HalfBandF64Sse2 <NC>::process_block_hpf (double out_ptr [], const double in_ptr [], long nbr_spl) noexcept
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
void	HalfBandF64Sse2 <NC>::process_sample_split (double &low, double &high, double input) noexcept
{
	const auto     y = process_2_paths (input);
	low  = y [0] + y [1];
	high = y [0] - y [1];
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
void	HalfBandF64Sse2 <NC>::process_block_split (double out_l_ptr [], double out_h_ptr [], const double in_ptr [], long nbr_spl) noexcept
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
void	HalfBandF64Sse2 <NC>::clear_buffers () noexcept
{
	for (int phase = 0; phase < _nbr_phases; ++phase)
	{
		for (int i = 0; i < _nbr_stages + 1; ++i)
		{
			_mm_store_pd (_bifilter [phase] [i]._mem, _mm_setzero_pd ());
		}
	}

	_prev  = 0;
	_phase = 0;
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int NC>
constexpr int	HalfBandF64Sse2 <NC>::_stage_width;
template <int NC>
constexpr int	HalfBandF64Sse2 <NC>::_nbr_stages;
template <int NC>
constexpr int	HalfBandF64Sse2 <NC>::_nbr_phases;



// Shared processing function, outputs both paths of the all-pass filter pair.
// Returns { even coefs, odd coefs }
template <int NC>
std::array <double, 2>	HalfBandF64Sse2 <NC>::process_2_paths (double input) noexcept
{
	auto           x    = _mm_set_pd (input, _prev);
	StageProcF64Sse2 <_nbr_stages>::process_sample_pos (
		x, &_bifilter [_phase] [0]
	);
	const auto     half = _mm_set1_pd (0.5);
	x = _mm_mul_pd (x, half);
	const auto     even = _mm_cvtsd_f64 (_mm_unpackhi_pd (x, x));
	const auto     odd  = _mm_cvtsd_f64 (x);

	_prev  = input;
	_phase = 1 - _phase;

	return { even, odd };
}



// One of the out_l_ptr or out_h_ptr can be nullptr
template <int NC>
template <typename FL, typename FH>
void	HalfBandF64Sse2 <NC>::process_block_2_paths (double out_l_ptr [], double out_h_ptr [], const double in_ptr [], long nbr_spl, FL fnc_l, FH fnc_h) noexcept
{
	assert (in_ptr != nullptr);
	assert (nbr_spl > 0);

	long           pos  = 0;
	const auto     half = _mm_set1_pd (0.5);

	if (_phase == 1)
	{
		const auto     path_arr = process_2_paths (in_ptr [0]);
		fnc_l (out_l_ptr, path_arr [0], path_arr [1]);
		fnc_h (out_h_ptr, path_arr [0], path_arr [1]);
		++ pos;
	}

	const long     end  = ((nbr_spl - pos) & -_nbr_phases) + pos;
	auto           prev = _mm_set1_pd (_prev);
	while (pos < end)
	{
		const auto     input_0 = _mm_set1_pd (in_ptr [pos    ]);
		const auto     input_1 = _mm_set1_pd (in_ptr [pos + 1]);

		auto           x_0 = _mm_unpacklo_pd (prev, input_0);
		auto           x_1 = _mm_unpacklo_pd (input_0, input_1); // prev = input_0

		StageProcF64Sse2 <_nbr_stages>::process_sample_pos (
			x_0, _bifilter [0].data ()
		);
		StageProcF64Sse2 <_nbr_stages>::process_sample_pos (
			x_1, _bifilter [1].data ()
		);

		x_0 = _mm_mul_pd (x_0, half);
		x_1 = _mm_mul_pd (x_1, half);

		const auto     even_0 = _mm_cvtsd_f64 (_mm_unpackhi_pd (x_0, x_0));
		const auto     odd_0  = _mm_cvtsd_f64 (x_0);
		const auto     even_1 = _mm_cvtsd_f64 (_mm_unpackhi_pd (x_1, x_1));
		const auto     odd_1  = _mm_cvtsd_f64 (x_1);
		fnc_l (out_l_ptr + pos    , even_0, odd_0);
		fnc_h (out_h_ptr + pos    , even_0, odd_0);
		fnc_l (out_l_ptr + pos + 1, even_1, odd_1);
		fnc_h (out_h_ptr + pos + 1, even_1, odd_1);

		prev = input_1;
		pos += 2;
	}
	_mm_store_sd (&_prev, prev);

	if (pos < nbr_spl)
	{
		assert (pos + 1 == nbr_spl);
		const auto     path_arr = process_2_paths (in_ptr [pos]);
		fnc_l (out_l_ptr + pos, path_arr [0], path_arr [1]);
		fnc_h (out_h_ptr + pos, path_arr [0], path_arr [1]);
	}
}



template <int NC>
void	HalfBandF64Sse2 <NC>::store_low (double *ptr, double even, double odd) noexcept
{
	*ptr = even + odd;
}



template <int NC>
void	HalfBandF64Sse2 <NC>::store_high (double *ptr, double even, double odd) noexcept
{
	*ptr = even - odd;
}



}  // namespace hiir



#endif   // hiir_HalfBandF64Sse2_CODEHEADER_INCLUDED

#undef hiir_HalfBandF64Sse2_CURRENT_CODEHEADER



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
