/*****************************************************************************

        Upsampler2x8F64Avx512.hpp
         Author: Laurent de Soras, 2020

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (hiir_Upsampler2x8F64Avx512_CODEHEADER_INCLUDED)
#define hiir_Upsampler2x8F64Avx512_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/StageProc8F64Avx512.h"

#include <cassert>



namespace hiir
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int NC>
constexpr int 	Upsampler2x8F64Avx512 <NC>::_nbr_chn;
template <int NC>
constexpr int 	Upsampler2x8F64Avx512 <NC>::NBR_COEFS;



/*
==============================================================================
Name: ctor
Throws: Nothing
==============================================================================
*/

template <int NC>
Upsampler2x8F64Avx512 <NC>::Upsampler2x8F64Avx512 () noexcept
:	_filter ()
{
	for (int i = 0; i < NBR_COEFS + 2; ++i)
	{
		_mm512_store_pd (_filter [i]._coef, _mm512_setzero_pd ());
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
void	Upsampler2x8F64Avx512 <NC>::set_coefs (const double coef_arr [NBR_COEFS]) noexcept
{
	assert (coef_arr != nullptr);

	for (int i = 0; i < NBR_COEFS; ++i)
	{
		_mm512_store_pd (
			_filter [i + 2]._coef, _mm512_set1_pd (DataType (coef_arr [i]))
		);
	}
}



/*
==============================================================================
Name: process_sample
Description:
	Upsamples (x2) the input vector, generating two output vectors.
Input parameters:
	- input: The input vector.
Output parameters:
	- out_0: First output vector.
	- out_1: Second output vector.
Throws: Nothing
==============================================================================
*/

template <int NC>
void	Upsampler2x8F64Avx512 <NC>::process_sample (__m512d &out_0, __m512d &out_1, __m512d input) noexcept
{
	__m512d         even = input;
	__m512d         odd  = input;
	StageProc8F64Avx512 <NBR_COEFS>::process_sample_pos (
		NBR_COEFS,
		even,
		odd,
		&_filter [0]
	);
	out_0 = even;
	out_1 = odd;
}



/*
==============================================================================
Name: process_block
Description:
	Upsamples (x2) the input vector block.
	Input and output blocks may overlap, see assert() for details.
Input parameters:
	- in_ptr: Input array, containing nbr_spl vector.
		No alignment constraint.
	- nbr_spl: Number of input vectors to process, > 0
Output parameters:
	- out_0_ptr: Output vector array, capacity: nbr_spl * 2 vectors.
		No alignment constraint.
Throws: Nothing
==============================================================================
*/

template <int NC>
void	Upsampler2x8F64Avx512 <NC>::process_block (double out_ptr [], const double in_ptr [], long nbr_spl) noexcept
{
	assert (out_ptr != nullptr);
	assert (in_ptr  != nullptr);
	assert (   out_ptr >= in_ptr + nbr_spl * _nbr_chn
	        || in_ptr >= out_ptr + nbr_spl * _nbr_chn);
	assert (nbr_spl > 0);

	long           pos = 0;
	do
	{
		__m512d         dst_0;
		__m512d         dst_1;
		const __m512d   src = _mm512_loadu_pd (in_ptr + pos * _nbr_chn);
		process_sample (dst_0, dst_1, src);
		_mm512_storeu_pd (out_ptr + pos * (_nbr_chn * 2)           , dst_0);
		_mm512_storeu_pd (out_ptr + pos * (_nbr_chn * 2) + _nbr_chn, dst_1);
		++ pos;
	}
	while (pos < nbr_spl);
}



/*
==============================================================================
Name: clear_buffers
Description:
	Clears filter memory, as if it proceAvx512d silence since an infinite amount
	of time.
Throws: Nothing
==============================================================================
*/

template <int NC>
void	Upsampler2x8F64Avx512 <NC>::clear_buffers () noexcept
{
	for (int i = 0; i < NBR_COEFS + 2; ++i)
	{
		_mm512_store_pd (_filter [i]._mem, _mm512_setzero_pd ());
	}
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}  // namespace hiir



#endif   // hiir_Upsampler2x8F64Avx512_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
