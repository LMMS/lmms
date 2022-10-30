/*****************************************************************************

        Upsampler2xSse.hpp
        Author: Laurent de Soras, 2020

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#if ! defined (hiir_Upsampler2xSse_CODEHEADER_INCLUDED)
#define hiir_Upsampler2xSse_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/StageProcSseV2.h"

#include <xmmintrin.h>

#include <cassert>



namespace hiir
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int NC>
constexpr int 	Upsampler2xSse <NC>::_nbr_chn;
template <int NC>
constexpr int 	Upsampler2xSse <NC>::NBR_COEFS;



/*
==============================================================================
Name: ctor
Throws: Nothing
==============================================================================
*/

template <int NC>
Upsampler2xSse <NC>::Upsampler2xSse () noexcept
:	_filter ()
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
void	Upsampler2xSse <NC>::set_coefs (const double coef_arr [NBR_COEFS]) noexcept
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
void	Upsampler2xSse <NC>::process_sample (float &out_0, float &out_1, float input) noexcept
{
	auto           x = _mm_set1_ps (input);
	StageProcSseV2 <_nbr_stages>::process_sample_pos (x, _filter.data ());
	out_1 = _mm_cvtss_f32 (x);
	x = _mm_shuffle_ps (x, x, 1);
	out_0 = _mm_cvtss_f32 (x);
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
void	Upsampler2xSse <NC>::process_block (float out_ptr [], const float in_ptr [], long nbr_spl) noexcept
{
	assert (out_ptr != nullptr);
	assert (in_ptr  != nullptr);
	assert (out_ptr >= in_ptr + nbr_spl || in_ptr >= out_ptr + nbr_spl);
	assert (nbr_spl > 0);

	const long     n4  = nbr_spl & ~(4-1);
	for (long pos = 0; pos < n4; pos += 4)
	{
#if 1
		auto           x_0 = _mm_set1_ps (in_ptr [pos    ]);
		auto           x_1 = _mm_set1_ps (in_ptr [pos + 1]);
		auto           x_2 = _mm_set1_ps (in_ptr [pos + 2]);
		auto           x_3 = _mm_set1_ps (in_ptr [pos + 3]);
#else // Same speed, actually
		const auto     x    = _mm_loadu_ps (in_ptr + pos); // a, b, c, d
		const auto     u_01 = _mm_unpacklo_ps (x, x); // a, a, b, b
		const auto     u_23 = _mm_unpackhi_ps (x, x); // c, c, d, d
		auto           x_0  = _mm_movelh_ps (u_01, u_01);
		auto           x_1  = _mm_movehl_ps (u_01, u_01);
		auto           x_2  = _mm_movelh_ps (u_23, u_23);
		auto           x_3  = _mm_movehl_ps (u_23, u_23);
#endif
		StageProcSseV2 <_nbr_stages>::process_sample_pos (x_0, _filter.data ());
		StageProcSseV2 <_nbr_stages>::process_sample_pos (x_1, _filter.data ());
		StageProcSseV2 <_nbr_stages>::process_sample_pos (x_2, _filter.data ());
		StageProcSseV2 <_nbr_stages>::process_sample_pos (x_3, _filter.data ());
		constexpr auto s_y = (1 << 0) | (0 << 2) | (1 << 4) | (0 << 6);
		const auto     y01 = _mm_shuffle_ps (x_0, x_1, s_y);
		const auto     y23 = _mm_shuffle_ps (x_2, x_3, s_y);
		_mm_storeu_ps (out_ptr + pos * 2    , y01);
		_mm_storeu_ps (out_ptr + pos * 2 + 4, y23);
	}
	for (long pos = n4; pos < nbr_spl; ++pos)
	{
		auto           x = _mm_set1_ps (in_ptr [pos]);
		StageProcSseV2 <_nbr_stages>::process_sample_pos (x, _filter.data ());
		x = _mm_shuffle_ps (x, x, 1);
		_mm_storel_pi (reinterpret_cast <__m64 *> (out_ptr + pos * 2), x);
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
void	Upsampler2xSse <NC>::clear_buffers () noexcept
{
	for (int i = 0; i < _nbr_stages + 1; ++i)
	{
		_mm_store_ps (_filter [i]._mem, _mm_setzero_ps ());
	}
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int NC>
constexpr int	Upsampler2xSse <NC>::_stage_width;
template <int NC>
constexpr int	Upsampler2xSse <NC>::_nbr_stages;



}  // namespace hiir



#endif   // hiir_Upsampler2xSse_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
