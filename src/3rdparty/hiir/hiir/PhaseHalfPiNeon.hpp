/*****************************************************************************

        PhaseHalfPiNeon.hpp
        Author: Laurent de Soras, 2016

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (hiir_PhaseHalfPiNeon_CODEHEADER_INCLUDED)
#define hiir_PhaseHalfPiNeon_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/fnc_neon.h"
#include "hiir/StageProcNeonV4.h"

#include <cassert>



namespace hiir
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int NC>
constexpr int 	PhaseHalfPiNeon <NC>::_nbr_chn;
template <int NC>
constexpr int 	PhaseHalfPiNeon <NC>::NBR_COEFS;
template <int NC>
constexpr double	PhaseHalfPiNeon <NC>::_delay;



/*
==============================================================================
Name: ctor
Throws: Nothing
==============================================================================
*/

template <int NC>
PhaseHalfPiNeon <NC>::PhaseHalfPiNeon () noexcept
:	_filter ()
,	_prev (0)
,	_phase (0)
{
	for (int phase = 0; phase < _nbr_phases; ++phase)
	{
		for (int i = 0; i < _nbr_stages + 1; ++i)
		{
			storea (_filter [phase] [i]._coef, vdupq_n_f32 (0));
		}
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
void	PhaseHalfPiNeon <NC>::set_coefs (const double coef_arr []) noexcept
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
	From one input sample, generates two samples with a pi/2 phase shift.
Input parameters:
	- input: The input sample.
Output parameters:
	- out_0: Output sample, ahead.
	- out_1: Output sample, late.
Throws: Nothing
==============================================================================
*/

template <int NC>
hiir_FORCEINLINE void	PhaseHalfPiNeon <NC>::process_sample (float &out_0, float &out_1, float input) noexcept
{
	StageDataNeonV4 * filter_ptr = &_filter [_phase] [0];

	const auto     comb    = vset_lane_f32 (input, vdup_n_f32 (_prev), 1);
	const auto     spl_mid =
#if ! defined (__BYTE_ORDER__) || (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
		// Requires a little-endian architecture, which is generally the case
		load2a (filter_ptr [_nbr_stages]._mem + 2);
#else
		// Safe on any platform, but possibly slower.
		vget_high_f32 (load4a (filter_ptr [_nbr_stages]._mem));
#endif
	auto           y       = vcombine_f32 (spl_mid, comb);
	auto           mem     = load4a (filter_ptr [0]._mem);

	StageProcNeonV4 <_nbr_stages>::process_sample_neg (&filter_ptr [0], y, mem);

	out_0  = vgetq_lane_f32 (y, 1);
	out_1  = vgetq_lane_f32 (y, 0);

	_prev  = input;
	_phase = 1 - _phase;
}



/*
==============================================================================
Name: process_block
Description:
	From a block of samples, generates two blocks of samples, with a pi/2
	phase shift between these signals.
	Input and output blocks may overlap, see assert() for details.
Input parameters:
	- in_ptr: Input array, containing nbr_spl samples.
	- nbr_spl: Number of input samples to process, > 0
Output parameters:
	- out_0_ptr: Output sample array (ahead), capacity: nbr_spl samples.
	- out_1_ptr: Output sample array (late), capacity: nbr_spl samples.
Throws: Nothing
==============================================================================
*/

template <int NC>
void	PhaseHalfPiNeon <NC>::process_block (float out_0_ptr [], float out_1_ptr [], const float in_ptr [], long nbr_spl) noexcept
{
	assert (out_0_ptr != nullptr);
	assert (out_1_ptr != nullptr);
	assert (in_ptr    != nullptr);
	assert (out_0_ptr <= in_ptr || out_0_ptr >= in_ptr + nbr_spl);
	assert (out_1_ptr <= in_ptr || out_1_ptr >= in_ptr + nbr_spl);
	assert (out_0_ptr + nbr_spl <= out_1_ptr || out_1_ptr + nbr_spl <= out_0_ptr);
	assert (nbr_spl > 0);

	if (_phase != 0)
	{
		process_sample (out_0_ptr [0], out_1_ptr [0], in_ptr [0]);
		++ out_0_ptr;
		++ out_1_ptr;
		++ in_ptr;
		-- nbr_spl;
	}

	if (nbr_spl > 0)
	{
		const long     n4 =
			process_block_quad (out_0_ptr, out_1_ptr, in_ptr, nbr_spl);

		for (long pos = n4; pos < nbr_spl; ++pos)
		{
			process_sample (out_0_ptr [pos], out_1_ptr [pos], in_ptr [pos]);
		}
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
void	PhaseHalfPiNeon <NC>::clear_buffers () noexcept
{
   for (int phase = 0; phase < _nbr_phases; ++phase)
   {
		for (int i = 0; i < _nbr_stages + 1; ++i)
		{
			storea (_filter [phase] [i]._mem, vdupq_n_f32 (0));
		}
	}
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int NC>
constexpr int	PhaseHalfPiNeon <NC>::_stage_width;
template <int NC>
constexpr int	PhaseHalfPiNeon <NC>::_nbr_stages;
template <int NC>
constexpr int	PhaseHalfPiNeon <NC>::_nbr_phases;
template <int NC>
constexpr int	PhaseHalfPiNeon <NC>::_coef_shift;



template <int NC>
void	PhaseHalfPiNeon <NC>::set_single_coef (int index, double coef) noexcept
{
	assert (index >= 0);
	assert (index < _nbr_stages * _stage_width);

	const int      stage = (index / _stage_width) + 1;
	const int      pos   = (index ^ _coef_shift) & (_stage_width - 1);
	for (int phase = 0; phase < _nbr_phases; ++phase)
	{
		_filter [phase] [stage]._coef [pos] = DataType (coef);
	}
}



template <int NC>
long	PhaseHalfPiNeon <NC>::process_block_quad (float out_0_ptr [], float out_1_ptr [], const float in_ptr [], long nbr_spl) noexcept
{
	assert (_phase == 0);

	const long     n4     = nbr_spl & ~(4-1);
	auto           prev   = _prev;
	auto           y_2    = load4a (_filter [0] [_nbr_stages]._mem);
	auto           y_3    = load4a (_filter [1] [_nbr_stages]._mem);
	for (long pos = 0; pos < n4; pos += 4)
	{
		const auto     x      = load4u (in_ptr + pos);

		const auto     comb_0 = float32x2_t { prev, vgetq_lane_f32 (x, 0) };
		auto           y_0    = vcombine_f32 (vget_high_f32 (y_2), comb_0);
		auto           mem_0  = load4a (_filter [0] [0]._mem);
		StageProcNeonV4 <_nbr_stages>::process_sample_neg (_filter [0].data (), y_0, mem_0);
		storea (_filter [0] [_nbr_stages]._mem, y_0);

		auto           y_1    = vcombine_f32 (vget_high_f32 (y_3), vget_low_f32 (x));
		auto           mem_1  = load4a (_filter [1] [0]._mem);
		StageProcNeonV4 <_nbr_stages>::process_sample_neg (_filter [1].data (), y_1, mem_1);
		storea (_filter [1] [_nbr_stages]._mem, y_1);

		const auto     comb_2 = float32x2_t { vgetq_lane_f32 (x, 1), vgetq_lane_f32 (x, 2) };
		               y_2    = vcombine_f32 (vget_high_f32 (y_0), comb_2);
		auto           mem_2  = load4a (_filter [0] [0]._mem);
		StageProcNeonV4 <_nbr_stages>::process_sample_neg (_filter [0].data (), y_2, mem_2);
		storea (_filter [0] [_nbr_stages]._mem, y_2);

		               y_3    = vcombine_f32 (vget_high_f32 (y_1), vget_high_f32 (x));
		auto           mem_3  = load4a (_filter [1] [0]._mem);
		StageProcNeonV4 <_nbr_stages>::process_sample_neg (_filter [1].data (), y_3, mem_3);
		storea (_filter [1] [_nbr_stages]._mem, y_3);

		prev = vgetq_lane_f32 (x, 3);

		const auto     u_02 = vcombine_f32 (vget_low_f32 (y_0), vget_low_f32 (y_2));
		const auto     u_13 = vcombine_f32 (vget_low_f32 (y_1), vget_low_f32 (y_3));
		const auto     both = vtrnq_f32 (u_02, u_13);
		const auto     odd  = both.val [0];
		const auto     even = both.val [1];
		storeu (out_0_ptr + pos, even);
		storeu (out_1_ptr + pos, odd);
	}
	_prev = prev;

	return n4;
}



}  // namespace hiir



#endif   // hiir_PhaseHalfPiNeon_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
