/*****************************************************************************

        Upsampler2xF64Sse2.hpp
        Author: Laurent de Soras, 2020

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#if ! defined (hiir_Upsampler2xF64Sse2_CODEHEADER_INCLUDED)
#define hiir_Upsampler2xF64Sse2_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/StageProcF64Sse2.h"

#include <cassert>



namespace hiir
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int NC>
constexpr int 	Upsampler2xF64Sse2 <NC>::_nbr_chn;
template <int NC>
constexpr int 	Upsampler2xF64Sse2 <NC>::NBR_COEFS;



/*
==============================================================================
Name: ctor
Throws: Nothing
==============================================================================
*/

template <int NC>
Upsampler2xF64Sse2 <NC>::Upsampler2xF64Sse2 () noexcept
:	_filter ()
{
	for (int i = 0; i < _nbr_stages + 1; ++i)
	{
		_mm_store_pd (_filter [i]._coef, _mm_setzero_pd ());
		_filter [i]._coef [1] = 0;
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
void	Upsampler2xF64Sse2 <NC>::set_coefs (const double coef_arr []) noexcept
{
	assert (coef_arr != nullptr);

	for (int i = 0; i < NBR_COEFS; ++i)
	{
		const int      stage = (i / _stage_width) + 1;
		const int      pos = (i ^ 1) & (_stage_width - 1);
		_filter [stage]._coef [pos] = DataType (coef_arr [i]);
	}
}



/*
==============================================================================
Name: process_sample
Description:
	Upsamples (x2) the input sample, generating two output samples.
Input parameters:
	- input: The input sample.
Output parameters:
	- out_0: First output sample.
	- out_1: Second output sample.
Throws: Nothing
==============================================================================
*/

template <int NC>
void	Upsampler2xF64Sse2 <NC>::process_sample (double &out_0, double &out_1, double input) noexcept
{
	auto           x = _mm_set1_pd (input);
	StageProcF64Sse2 <_nbr_stages>::process_sample_pos (x, &_filter [0]);
	_mm_storel_pd (&out_1, x);
	_mm_storeh_pd (&out_0, x);
}



/*
==============================================================================
Name: process_block
Description:
	Upsamples (x2) the input sample block.
	Input and output blocks may overlap, see assert() for details.
Input parameters:
	- in_ptr: Input array, containing nbr_spl samples.
	- nbr_spl: Number of input samples to process, > 0
Output parameters:
	- out_0_ptr: Output sample array, capacity: nbr_spl * 2 samples.
Throws: Nothing
==============================================================================
*/

template <int NC>
void	Upsampler2xF64Sse2 <NC>::process_block (double out_ptr [], const double in_ptr [], long nbr_spl) noexcept
{
	assert (out_ptr != nullptr);
	assert (in_ptr  != nullptr);
	assert (out_ptr >= in_ptr + nbr_spl || in_ptr >= out_ptr + nbr_spl);
	assert (nbr_spl > 0);

	const long     n2 = process_block_double (out_ptr, in_ptr, nbr_spl);
	if (n2 < nbr_spl)
	{
		process_sample (out_ptr [n2 * 2], out_ptr [n2 * 2 + 1], in_ptr [n2]);
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
void	Upsampler2xF64Sse2 <NC>::clear_buffers () noexcept
{
	for (int i = 0; i < _nbr_stages + 1; ++i)
	{
		_mm_store_pd (_filter [i]._mem, _mm_setzero_pd ());
	}
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int NC>
constexpr int	Upsampler2xF64Sse2 <NC>::_stage_width;
template <int NC>
constexpr int	Upsampler2xF64Sse2 <NC>::_nbr_stages;



template <int NC>
long	Upsampler2xF64Sse2 <NC>::process_block_double (double out_ptr [], const double in_ptr [], long nbr_spl) noexcept
{
	const long     n2 = nbr_spl & ~(2-1);
	for (long pos = 0; pos < n2; pos += 2)
	{
		const auto     x   = _mm_loadu_pd (in_ptr + pos);
		auto           x_0 = _mm_unpacklo_pd (x, x);
		auto           x_1 = _mm_unpackhi_pd (x, x);
		StageProcF64Sse2 <_nbr_stages>::process_sample_pos (x_0, _filter.data ());
		StageProcF64Sse2 <_nbr_stages>::process_sample_pos (x_1, _filter.data ());
		const auto     y_0 = _mm_shuffle_pd (x_0, x_0, 1);
		const auto     y_1 = _mm_shuffle_pd (x_1, x_1, 1);
		_mm_storeu_pd (out_ptr + pos * 2    , y_0);
		_mm_storeu_pd (out_ptr + pos * 2 + 2, y_1);
	}

	return n2;
}



}  // namespace hiir



#endif   // hiir_Upsampler2xF64Sse2_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
