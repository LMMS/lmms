/*****************************************************************************

        PhaseHalfPi3dnow.hpp
        Author: Laurent de Soras, 2005

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if defined (hiir_PhaseHalfPi3dnow_CURRENT_CODEHEADER)
	#error Recursive inclusion of PhaseHalfPi3dnow code header.
#endif
#define	hiir_PhaseHalfPi3dnow_CURRENT_CODEHEADER

#if ! defined (hiir_PhaseHalfPi3dnow_CODEHEADER_INCLUDED)
#define	hiir_PhaseHalfPi3dnow_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/StageProc3dnow.h"

#include <mm3dnow.h>

#include <cassert>



#if defined (_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4740)
#endif



namespace hiir
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int NC>
constexpr int 	PhaseHalfPi3dnow <NC>::_nbr_chn;
template <int NC>
constexpr int 	PhaseHalfPi3dnow <NC>::NBR_COEFS;
template <int NC>
constexpr double	PhaseHalfPi3dnow <NC>::_delay;



/*
==============================================================================
Name: ctor
Throws: Nothing
==============================================================================
*/

template <int NC>
PhaseHalfPi3dnow <NC>::PhaseHalfPi3dnow () noexcept
:	_filter ()
,	_prev (0)
,	_phase (0)
{
   for (int phase = 0; phase < _nbr_phases; ++phase)
   {
	   for (int i = 0; i < _nbr_stages + 1; ++i)
	   {
		   _filter [phase] [i]._coefs.m64_f32 [0] = 0;
		   _filter [phase] [i]._coefs.m64_f32 [1] = 0;
	   }
	   if (NBR_COEFS < _nbr_stages * 2)
	   {
		   _filter [phase] [_nbr_stages]._coefs.m64_f32 [0] = 1;
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
void	PhaseHalfPi3dnow <NC>::set_coefs (const double coef_arr []) noexcept
{
	assert (coef_arr != nullptr);

   for (int phase = 0; phase < _nbr_phases; ++phase)
   {
	   for (int i = 0; i < NBR_COEFS; ++i)
	   {
		   const int      stage = (i / _stage_width) + 1;
		   const int      pos   = (i ^ 1) & (_stage_width - 1);
		   _filter [phase] [stage]._coefs.m64_f32 [pos] = DataType (coef_arr [i]);
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
void	PhaseHalfPi3dnow <NC>::process_sample (float &out_0, float &out_1, float input) noexcept
{
	constexpr int  CURR_CELL = _nbr_stages * sizeof (_filter [0] [0]);

	StageData3dnow *  filter_ptr = &_filter [_phase] [0];
   __m64           result;

   result.m64_f32 [0] = _prev;
   result.m64_f32 [1] = input;

	__asm
	{
		mov            edx, filter_ptr
		movq           mm0, result
	}
	StageProc3dnow <_nbr_stages>::process_sample_neg ();
	__asm
	{
		movq           [edx + CURR_CELL + 1*8], mm0

		femms
	}

   out_0 = filter_ptr [_nbr_stages]._mem.m64_f32 [1];
   out_1 = filter_ptr [_nbr_stages]._mem.m64_f32 [0];

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

#if defined (_MSC_VER)
#pragma warning (push)
#pragma warning (4 : 4731 4748)
#endif

template <int NC>
void	PhaseHalfPi3dnow <NC>::process_block (float out_0_ptr [], float out_1_ptr [], const float in_ptr [], long nbr_spl) noexcept
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

	if (pos < nbr_spl)
	{
		float          prev = _prev;

		constexpr int  CURR_CELL = _nbr_stages * sizeof (_filter [0] [0]);

		StageData3dnow *  filter_ptr = &_filter [0] [0];
		StageData3dnow *  filter2_ptr = &_filter [1] [0];

		__asm
		{
			push           ebx

			mov            esi, in_ptr
			mov            edi, out_0_ptr
			mov            ecx, out_1_ptr
			mov            eax, nbr_spl
			mov            edx, filter_ptr
			mov            ebx, filter2_ptr
			lea            esi, [esi + eax*4]
			lea            edi, [edi + eax*4 - 4]
			lea            ecx, [ecx + eax*4 - 4]
			neg            eax
			add            eax, pos
			movd           mm0, prev

		loop_sample:

			movd           mm3, [esi + eax*4]
			punpckldq      mm0, mm3
		}
#if defined (_MSC_VER) && ! defined (NDEBUG)
		__asm push        eax
		__asm push        ecx
#endif
		StageProc3dnow <_nbr_stages>::process_sample_neg ();
#if defined (_MSC_VER) && ! defined (NDEBUG)
		__asm pop         ecx
		__asm pop         eax
#endif
		__asm
		{
			inc            eax
			movq           [edx + CURR_CELL + 1*8], mm0
			xchg           edx, ebx
			movd           [ecx + eax*4], mm0
			punpckhdq      mm0, mm0
			movd           [edi + eax*4], mm0

			movq           mm0, mm3

			jl             loop_sample

			movd           prev, mm3

			femms
			pop            ebx
		}

		_phase = (nbr_spl - pos) & 1;
		_prev  = prev;
	}
}

#if defined (_MSC_VER)
#pragma warning (pop)
#endif



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
void	PhaseHalfPi3dnow <NC>::clear_buffers () noexcept
{
	for (int phase = 0; phase < _nbr_phases; ++phase)
	{
		for (int i = 0; i < _nbr_stages + 1; ++i)
		{
			_filter [phase] [i]._mem.m64_f32 [0] = 0;
			_filter [phase] [i]._mem.m64_f32 [1] = 0;
		}
	}
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int NC>
constexpr int	PhaseHalfPi3dnow <NC>::_stage_width;
template <int NC>
constexpr int	PhaseHalfPi3dnow <NC>::_nbr_stages;
template <int NC>
constexpr int	PhaseHalfPi3dnow <NC>::_nbr_phases;



}  // namespace hiir



#if defined (_MSC_VER)
#pragma warning (pop)
#endif



#endif   // hiir_PhaseHalfPi3dnow_CODEHEADER_INCLUDED

#undef hiir_PhaseHalfPi3dnow_CURRENT_CODEHEADER



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
