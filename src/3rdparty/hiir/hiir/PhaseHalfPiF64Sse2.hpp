/*****************************************************************************

        PhaseHalfPiF64Sse2.hpp
        Author: Laurent de Soras, 2020

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#if ! defined (hiir_PhaseHalfPiF64Sse2_CODEHEADER_INCLUDED)
#define hiir_PhaseHalfPiF64Sse2_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/StageProcF64Sse2.h"

#include <cassert>



namespace hiir
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int NC>
constexpr int 	PhaseHalfPiF64Sse2 <NC>::_nbr_chn;
template <int NC>
constexpr int 	PhaseHalfPiF64Sse2 <NC>::NBR_COEFS;
template <int NC>
constexpr double	PhaseHalfPiF64Sse2 <NC>::_delay;



/*
==============================================================================
Name: ctor
Throws: Nothing
==============================================================================
*/

template <int NC>
PhaseHalfPiF64Sse2 <NC>::PhaseHalfPiF64Sse2 () noexcept
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
void	PhaseHalfPiF64Sse2 <NC>::set_coefs (const double coef_arr []) noexcept
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
void	PhaseHalfPiF64Sse2 <NC>::process_sample (double &out_0, double &out_1, double input) noexcept
{
	auto           x = _mm_set_pd (input, _prev);
	StageProcF64Sse2 <_nbr_stages>::process_sample_neg (
		x, &_bifilter [_phase] [0]
	);
	out_0 = _mm_cvtsd_f64 (_mm_unpackhi_pd (x, x));
	out_1 = _mm_cvtsd_f64 (x);

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
void	PhaseHalfPiF64Sse2 <NC>::process_block (double out_0_ptr [], double out_1_ptr [], const double in_ptr [], long nbr_spl) noexcept
{
	assert (out_0_ptr != nullptr);
	assert (out_1_ptr != nullptr);
	assert (in_ptr    != nullptr);
	assert (out_0_ptr <= in_ptr || out_0_ptr >= in_ptr + nbr_spl);
	assert (out_1_ptr <= in_ptr || out_1_ptr >= in_ptr + nbr_spl);
	assert (out_0_ptr + nbr_spl <= out_1_ptr || out_1_ptr + nbr_spl <= out_0_ptr);
	assert (nbr_spl > 0);

	long           pos = 0;
	if (_phase == 1)
	{
		process_sample (out_0_ptr [0], out_1_ptr [0], in_ptr [0]);
		++ pos;
	}

	const long     end  = ((nbr_spl - pos) & -_nbr_phases) + pos;
	auto           prev = _mm_set1_pd (_prev);
	while (pos < end)
	{
		const auto     input_0 = _mm_set1_pd (in_ptr [pos    ]);
		auto           x_0     = _mm_shuffle_pd (prev, input_0, 1);
		StageProcF64Sse2 <_nbr_stages>::process_sample_neg (
			x_0, &_bifilter [0] [0]
		);

		const auto     input_1 = _mm_set1_pd (in_ptr [pos + 1]);
		auto           x_1 = _mm_shuffle_pd (input_0, input_1, 1); // prev = input_0
		StageProcF64Sse2 <_nbr_stages>::process_sample_neg (
			x_1, &_bifilter [1] [0]
		);

		const auto     y_0 = _mm_unpackhi_pd (x_0, x_1);
		const auto     y_1 = _mm_unpacklo_pd (x_0, x_1);
		_mm_storeu_pd (out_0_ptr + pos, y_0);
		_mm_storeu_pd (out_1_ptr + pos, y_1);

		pos += 2;
		prev = input_1;
	}
	_mm_store_sd (&_prev, prev);

	if (pos < nbr_spl)
	{
		assert (pos + 1 == nbr_spl);
		process_sample (out_0_ptr [pos], out_1_ptr [pos], in_ptr [pos]);
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
void	PhaseHalfPiF64Sse2 <NC>::clear_buffers () noexcept
{
	for (int phase = 0; phase < _nbr_phases; ++phase)
	{
		for (int i = 0; i < _nbr_stages + 1; ++i)
		{
			_mm_store_pd (_bifilter [phase] [i]._mem, _mm_setzero_pd ());
		}
	}
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int NC>
constexpr int	PhaseHalfPiF64Sse2 <NC>::_stage_width;
template <int NC>
constexpr int	PhaseHalfPiF64Sse2 <NC>::_nbr_stages;
template <int NC>
constexpr int	PhaseHalfPiF64Sse2 <NC>::_nbr_phases;



}  // namespace hiir



#endif   // hiir_PhaseHalfPiF64Sse2_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
