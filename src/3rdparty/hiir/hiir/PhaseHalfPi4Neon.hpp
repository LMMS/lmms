/*****************************************************************************

        PhaseHalfPi4Neon.hpp
        Author: Laurent de Soras, 2020

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#if ! defined (hiir_PhaseHalfPi4Neon_CODEHEADER_INCLUDED)
#define hiir_PhaseHalfPi4Neon_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/fnc_neon.h"
#include "hiir/StageProc4Neon.h"

#include <cassert>



namespace hiir
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int NC>
constexpr int 	PhaseHalfPi4Neon <NC>::_nbr_chn;
template <int NC>
constexpr int 	PhaseHalfPi4Neon <NC>::NBR_COEFS;
template <int NC>
constexpr double	PhaseHalfPi4Neon <NC>::_delay;



/*
==============================================================================
Name: ctor
Throws: Nothing
==============================================================================
*/

template <int NC>
PhaseHalfPi4Neon <NC>::PhaseHalfPi4Neon () noexcept
:	_bifilter ()
,	_phase (0)
{
	for (int i = 0; i < NBR_COEFS + 2; ++i)
	{
		storea (_bifilter [0] [i]._coef, vdupq_n_f32 (0));
		storea (_bifilter [1] [i]._coef, vdupq_n_f32 (0));
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
void	PhaseHalfPi4Neon <NC>::set_coefs (const double coef_arr []) noexcept
{
	assert (coef_arr != nullptr);

	for (int i = 0; i < NBR_COEFS; ++i)
	{
		const float32x4_t c = vdupq_n_f32 (DataType (coef_arr [i]));
		storea (_bifilter [0] [i + 2]._coef, c);
		storea (_bifilter [1] [i + 2]._coef, c);
	}
}



/*
==============================================================================
Name: process_sample
Description:
	From one 4-channel input vector, generates two vectors with a pi/2 phase
	shift between these signals.
Input parameters:
	- input: The input sample.
Output parameters:
	- out_0: Output sample, ahead.
	- out_1: Output sample, late.
Throws: Nothing
==============================================================================
*/

template <int NC>
void	PhaseHalfPi4Neon <NC>::process_sample (float32x4_t &out_0, float32x4_t &out_1, float32x4_t input) noexcept
{
	out_0 = input;          // Even coefs
	out_1 = load4a (_prev); // Odd coefs

	StageProc4Neon <NBR_COEFS>::process_sample_neg (
		NBR_COEFS, out_0, out_1, &_bifilter [_phase] [0]
	);

	storea (_prev, input);
	_phase = 1 - _phase;
}



/*
==============================================================================
Name: process_block
Description:
	From a block of 4-channel input vectors, generates two blocks of vectors
	with a pi/2 phase shift between these signals.
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
void	PhaseHalfPi4Neon <NC>::process_block (float out_0_ptr [], float out_1_ptr [], const float in_ptr [], long nbr_spl) noexcept
{
	assert (out_0_ptr != nullptr);
	assert (out_1_ptr != nullptr);
	assert (in_ptr    != nullptr);
	assert (out_0_ptr <= in_ptr || out_0_ptr >= in_ptr + nbr_spl * _nbr_chn);
	assert (out_1_ptr <= in_ptr || out_1_ptr >= in_ptr + nbr_spl * _nbr_chn);
	assert (   out_0_ptr + nbr_spl * _nbr_chn <= out_1_ptr
	        || out_1_ptr + nbr_spl * _nbr_chn <= out_0_ptr);
	assert (nbr_spl > 0);

	long           pos = 0;
	if (_phase == 1)
	{
		float32x4_t    out_0;
		float32x4_t    out_1;
		process_sample (out_0, out_1, load4u (in_ptr));
		storeu (out_0_ptr, out_0);
		storeu (out_1_ptr, out_1);
		++ pos;
	}

	const long     end  = ((nbr_spl - pos) & -_nbr_phases) + pos;
	float32x4_t    prev = load4a (_prev);
	while (pos < end)
	{
		const auto     ofs_0   = pos * _nbr_chn;
		const auto     input_0 = load4u (in_ptr + ofs_0);
		auto           out_0   = input_0;
		auto           out_1   = prev;
		StageProc4Neon <NBR_COEFS>::process_sample_neg (
			NBR_COEFS, out_0, out_1, &_bifilter [0] [0]
		);
		storeu (out_0_ptr + ofs_0, out_0);
		storeu (out_1_ptr + ofs_0, out_1);

		const auto     ofs_1   = ofs_0 + _nbr_chn;
		const auto     input_1 = load4u (in_ptr + ofs_1);
		out_0 = input_1;
		out_1 = input_0;  // prev
		StageProc4Neon <NBR_COEFS>::process_sample_neg (
			NBR_COEFS, out_0, out_1, &_bifilter [1] [0]
		);
		storeu (out_0_ptr + ofs_1, out_0);
		storeu (out_1_ptr + ofs_1, out_1);

		prev = input_1;
		pos += 2;
	}
	storea (_prev, prev);

	if (pos < nbr_spl)
	{
		assert (pos + 1 == nbr_spl);
		float32x4_t    out_0;
		float32x4_t    out_1;
		const auto     ofs = pos * _nbr_chn;
		process_sample (out_0, out_1, load4u (in_ptr + ofs));
		storeu (out_0_ptr + ofs, out_0);
		storeu (out_1_ptr + ofs, out_1);
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
void	PhaseHalfPi4Neon <NC>::clear_buffers () noexcept
{
	_phase = 0;
	for (int i = 0; i < NBR_COEFS + 2; ++i)
	{
		storea (_bifilter [0] [i]._mem, vdupq_n_f32 (0));
		storea (_bifilter [1] [i]._mem, vdupq_n_f32 (0));
	}
	storea (_prev, vdupq_n_f32 (0));
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int NC>
constexpr int	PhaseHalfPi4Neon <NC>::_nbr_phases;



}  // namespace hiir



#endif   // hiir_PhaseHalfPi4Neon_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

