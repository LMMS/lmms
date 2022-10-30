/*****************************************************************************

        Upsampler2x3dnow.hpp
        Author: Laurent de Soras, 2005

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if defined (hiir_Upsampler2x3dnow_CURRENT_CODEHEADER)
	#error Recursive inclusion of Upsampler2x3dnow code header.
#endif
#define hiir_Upsampler2x3dnow_CURRENT_CODEHEADER

#if ! defined (hiir_Upsampler2x3dnow_CODEHEADER_INCLUDED)
#define	hiir_Upsampler2x3dnow_CODEHEADER_INCLUDED



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
constexpr int 	Upsampler2x3dnow <NC>::_nbr_chn;
template <int NC>
constexpr int 	Upsampler2x3dnow <NC>::NBR_COEFS;



/*
==============================================================================
Name: ctor
Throws: Nothing
==============================================================================
*/

template <int NC>
Upsampler2x3dnow <NC>::Upsampler2x3dnow () noexcept
:	_filter ()
{
	for (int i = 0; i < _nbr_stages + 1; ++i)
	{
		_filter [i]._coefs.m64_f32 [0] = 0;
		_filter [i]._coefs.m64_f32 [1] = 0;
	}
	if (NBR_COEFS < _nbr_stages * 2)
	{
		_filter [_nbr_stages]._coefs.m64_f32 [0] = 1;
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
void	Upsampler2x3dnow <NC>::set_coefs (const double coef_arr [NBR_COEFS]) noexcept
{
	assert (coef_arr != nullptr);

	for (int i = 0; i < NBR_COEFS; ++i)
	{
		const int      stage = (i / _stage_width) + 1;
		const int      pos = (i ^ 1) & (_stage_width - 1);
		_filter [stage]._coefs.m64_f32 [pos] = DataType (coef_arr [i]);
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
void	Upsampler2x3dnow <NC>::process_sample (float &out_0, float &out_1, float input) noexcept
{
	constexpr int  CURR_CELL = _nbr_stages * sizeof (_filter [0]);

	StageData3dnow *  filter_ptr = &_filter [0];
	__m64          result;

	__asm
	{
		movd           mm0, input
		mov            edx, filter_ptr
		punpckldq      mm0, mm0
	}
	StageProc3dnow <_nbr_stages>::process_sample_pos ();
	__asm
	{
		movq           [edx + CURR_CELL + 1*8], mm0

		femms
	}

	out_0 = _filter [_nbr_stages]._mem.m64_f32 [1];
	out_1 = _filter [_nbr_stages]._mem.m64_f32 [0];
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
void	Upsampler2x3dnow <NC>::process_block (float out_ptr [], const float in_ptr [], long nbr_spl) noexcept
{
	assert (out_ptr != nullptr);
	assert (in_ptr  != nullptr);
	assert (out_ptr >= in_ptr + nbr_spl || in_ptr >= out_ptr + nbr_spl);
	assert (nbr_spl > 0);

	constexpr int  CURR_CELL = _nbr_stages * sizeof (_filter [0]);

	StageData3dnow *  filter_ptr = &_filter [0];

	__asm
	{
		mov            esi, in_ptr
		mov            edi, out_ptr
		mov            eax, nbr_spl
		mov            edx, filter_ptr
		lea            esi, [esi + eax*4]
		lea            edi, [edi + eax*8 - 8]
		neg            eax

	loop_sample:

		movd           mm0, [esi + eax*4]
		punpckldq      mm0, mm0
	}
#if defined (_MSC_VER) && ! defined (NDEBUG)
	__asm push        eax
#endif
	StageProc3dnow <_nbr_stages>::process_sample_pos ();
#if defined (_MSC_VER) && ! defined (NDEBUG)
	__asm pop         eax
#endif
	__asm
	{
		inc            eax
		movq           [edx + CURR_CELL + 1*8], mm0
		movd           [edi + eax*8 + 4], mm0
		punpckhdq      mm0, mm0
		movd           [edi + eax*8    ], mm0

		jl             loop_sample

		femms
	}
	// pswapd + movq would have been better to store result (but only for athlon)
}

// We could write the same specialisation for <7>
template <>
void	Upsampler2x3dnow <8>::process_block (float out_ptr [], const float in_ptr [], long nbr_spl) noexcept
{
	StageData3dnow *	filter_ptr = &_filter [0];

	__asm
	{
		mov            esi, in_ptr
		mov            edi, out_ptr
		mov            eax, nbr_spl
		lea            esi, [esi + eax*4]
		lea            edi, [edi + eax*8 - 8]
		neg            eax
		mov            edx, filter_ptr

		movq           mm2, [edx + 0*16 + 1*8]
		movq           mm3, [edx + 1*16 + 1*8]
		movq           mm4, [edx + 2*16 + 1*8]
		movq           mm5, [edx + 3*16 + 1*8]
		movq           mm6, [edx + 4*16 + 1*8]

	loop_sample:

		movd           mm0, [esi + eax*4]
		punpckldq      mm0, mm0

		movq           mm1, mm2
		movq           mm2, mm0
		pfsub          mm0, mm3
		pfmul          mm0, [edx + 1*16 + 0*8]
		inc            eax
		pfadd          mm0, mm1

		movq           mm1, mm3
		movq           mm3, mm0
		pfsub          mm0, mm4
		pfmul          mm0, [edx + 2*16 + 0*8]
		pfadd          mm0, mm1

		movq           mm1, mm4
		movq           mm4, mm0
		pfsub          mm0, mm5
		pfmul          mm0, [edx + 3*16 + 0*8]
		pfadd          mm0, mm1

		movq           mm1, mm5
		movq           mm5, mm0
		pfsub          mm0, mm6
		pfmul          mm0, [edx + 4*16 + 0*8]
		pfadd          mm0, mm1

		movq           mm6, mm0

		movd           [edi + eax*8 + 4], mm0
		punpckhdq      mm0, mm0
		movd           [edi + eax*8    ], mm0

		jl             loop_sample

		movq           [edx + 0*16 + 1*8], mm2
		movq           [edx + 1*16 + 1*8], mm3
		movq           [edx + 2*16 + 1*8], mm4
		movq           [edx + 3*16 + 1*8], mm5
		movq           [edx + 4*16 + 1*8], mm6

		femms
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
void	Upsampler2x3dnow <NC>::clear_buffers () noexcept
{
	for (int i = 0; i < _nbr_stages + 1; ++i)
	{
		_filter [i]._mem.m64_f32 [0] = 0;
		_filter [i]._mem.m64_f32 [1] = 0;
	}
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int NC>
constexpr int	Upsampler2x3dnow <NC>::_stage_width;
template <int NC>
constexpr int	Upsampler2x3dnow <NC>::_nbr_stages;



}  // namespace hiir



#if defined (_MSC_VER)
#pragma warning (pop)
#endif



#endif   // hiir_Upsampler2x3dnow_CODEHEADER_INCLUDED

#undef hiir_Upsampler2x3dnow_CURRENT_CODEHEADER



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
