/*****************************************************************************

        Upsampler2xNeonOld.hpp
        Author: Laurent de Soras, 2016

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (hiir_Upsampler2xNeonOld_CODEHEADER_INCLUDED)
#define hiir_Upsampler2xNeonOld_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/StageProcNeonV4.h"

#include <arm_neon.h>

#include <cassert>



namespace hiir
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int NC>
constexpr int	Upsampler2xNeonOld <NC>::_nbr_chn;
template <int NC>
constexpr int	Upsampler2xNeonOld <NC>::NBR_COEFS;



/*
==============================================================================
Name: ctor
Throws: Nothing
==============================================================================
*/

template <int NC>
Upsampler2xNeonOld <NC>::Upsampler2xNeonOld () noexcept
:	_filter ()
{
	for (int i = 0; i < _nbr_stages + 1; ++i)
	{
		storea (_filter [i]._coef, vdupq_n_f32 (0));
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
void	Upsampler2xNeonOld <NC>::set_coefs (const double coef_arr [NBR_COEFS]) noexcept
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
void	Upsampler2xNeonOld <NC>::process_sample (float &out_0, float &out_1, float input) noexcept
{
	const auto     spl_in = vdup_n_f32 (input);
	const auto     spl_m  = vget_high_f32 (load4a (_filter [_nbr_stages]._mem));
	auto           y      = vcombine_f32 (spl_m, spl_in);
	auto           mem    = load4a (_filter [0]._mem);

	StageProcNeonV4 <_nbr_stages>::process_sample_pos (&_filter [0], y, mem);

	out_0 = vgetq_lane_f32 (y, 1);
	out_1 = vgetq_lane_f32 (y, 0);
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
void	Upsampler2xNeonOld <NC>::process_block (float out_ptr [], const float in_ptr [], long nbr_spl) noexcept
{
	assert (out_ptr != nullptr);
	assert (in_ptr  != nullptr);
	assert (out_ptr >= in_ptr + nbr_spl || in_ptr >= out_ptr + nbr_spl);
	assert (nbr_spl > 0);

	const long     n4 = process_block_quad (out_ptr, in_ptr, nbr_spl);

	for (long pos = n4; pos < nbr_spl; ++pos)
	{
		process_sample (out_ptr [pos * 2], out_ptr [pos * 2 + 1], in_ptr [pos]);
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
void	Upsampler2xNeonOld <NC>::clear_buffers () noexcept
{
	for (int i = 0; i < _nbr_stages + 1; ++i)
	{
		storea (_filter [i]._mem, vdupq_n_f32 (0));
	}
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int NC>
constexpr int	Upsampler2xNeonOld <NC>::_stage_width;
template <int NC>
constexpr int	Upsampler2xNeonOld <NC>::_nbr_stages;
template <int NC>
constexpr int	Upsampler2xNeonOld <NC>::_coef_shift;



template <int NC>
void	Upsampler2xNeonOld <NC>::set_single_coef (int index, double coef) noexcept
{
	assert (index >= 0);
	assert (index < _nbr_stages * _stage_width);

	const int      stage = (index / _stage_width) + 1;
	const int      pos   = (index ^ _coef_shift) & (_stage_width - 1);
	_filter [stage]._coef [pos] = DataType (coef);
}



template <int NC>
long	Upsampler2xNeonOld <NC>::process_block_quad (float out_ptr [], const float in_ptr [], long nbr_spl) noexcept
{
	const long     n4   = nbr_spl & ~(4-1);
	auto           y_3  = load4a (_filter [_nbr_stages]._mem);
	for (long pos = 0; pos < n4; pos += 4)
	{
		const auto     x      = load4u (in_ptr + pos);

		auto           y_0    = vcombine_f32 (
			vget_high_f32 (y_3), vdup_lane_f32 (vget_low_f32 (x), 0)
		);
		auto           mem_0  = load4a (_filter [0]._mem);
		StageProcNeonV4 <_nbr_stages>::process_sample_pos (_filter.data (), y_0, mem_0);
		storea (_filter [_nbr_stages]._mem, y_0);

		auto           y_1    = vcombine_f32 (
			vget_high_f32 (y_0), vdup_lane_f32 (vget_low_f32 (x), 1)
		);
		auto           mem_1  = load4a (_filter [0]._mem);
		StageProcNeonV4 <_nbr_stages>::process_sample_pos (_filter.data (), y_1, mem_1);
		storea (_filter [_nbr_stages]._mem, y_1);

		auto           y_2    = vcombine_f32 (
			vget_high_f32 (y_1), vdup_lane_f32 (vget_high_f32 (x), 0)
		);
		auto           mem_2  = load4a (_filter [0]._mem);
		StageProcNeonV4 <_nbr_stages>::process_sample_pos (_filter.data (), y_2, mem_2);
		storea (_filter [_nbr_stages]._mem, y_2);

		               y_3    = vcombine_f32 (
			vget_high_f32 (y_2), vdup_lane_f32 (vget_high_f32 (x), 1)
		);
		auto           mem_3  = load4a (_filter [0]._mem);
		StageProcNeonV4 <_nbr_stages>::process_sample_pos (_filter.data (), y_3, mem_3);
		storea (_filter [_nbr_stages]._mem, y_3);

		const auto    y_01 = vcombine_f32 (
			vrev64_f32 (vget_low_f32 (y_0)), vrev64_f32 (vget_low_f32 (y_1))
		);
		const auto    y_23 = vcombine_f32 (
			vrev64_f32 (vget_low_f32 (y_2)), vrev64_f32 (vget_low_f32 (y_3))
		);
		storeu (out_ptr + pos * 2    , y_01);
		storeu (out_ptr + pos * 2 + 4, y_23);
	}

	return n4;
}



}  // namespace hiir



#endif   // hiir_Upsampler2xNeonOld_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
