/*****************************************************************************

        Downsampler2x3dnow.hpp
        Author: Laurent de Soras, 2005

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if defined (hiir_Downsampler2x3dnow_CURRENT_CODEHEADER)
	#error Recursive inclusion of Downsampler2x3dnow code header.
#endif
#define hiir_Downsampler2x3dnow_CURRENT_CODEHEADER

#if ! defined (hiir_Downsampler2x3dnow_CODEHEADER_INCLUDED)
#define hiir_Downsampler2x3dnow_CODEHEADER_INCLUDED



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



static const float	Downsampler2x3dnow_half [2] = { 0.5f, 0.5f };



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int NC>
constexpr int 	Downsampler2x3dnow <NC>::_nbr_chn;
template <int NC>
constexpr int 	Downsampler2x3dnow <NC>::NBR_COEFS;
template <int NC>
constexpr double	Downsampler2x3dnow <NC>::_delay;



/*
==============================================================================
Name: ctor
Throws: Nothing
==============================================================================
*/

template <int NC>
Downsampler2x3dnow <NC>::Downsampler2x3dnow () noexcept
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
void	Downsampler2x3dnow <NC>::set_coefs (const double coef_arr []) noexcept
{
	assert (coef_arr != nullptr);

	for (int i = 0; i < NBR_COEFS; ++i)
	{
		const int      stage = (i / _stage_width) + 1;
		const int      pos   = (i ^ 1) & (_stage_width - 1);
		_filter [stage]._coefs.m64_f32 [pos] = DataType (coef_arr [i]);
	}
}



/*
==============================================================================
Name: process_sample
Description:
	Downsamples (x2) one pair of samples, to generate one output sample.
Input parameters:
	- in_ptr: pointer on the two samples to decimate
Returns: Samplerate-reduced sample.
Throws: Nothing
==============================================================================
*/

template <int NC>
float	Downsampler2x3dnow <NC>::process_sample (const float in_ptr [2]) noexcept
{
	assert (in_ptr != nullptr);

	constexpr int  CURR_CELL = _nbr_stages * sizeof (_filter [0]);

	StageData3dnow *  filter_ptr = &_filter [0];
	float           result;

	__asm
	{
		mov            esi, in_ptr
		mov            edx, filter_ptr
		movq           mm0, [esi]
	}
	StageProc3dnow <_nbr_stages>::process_sample_pos ();
	__asm
	{
		movq           [edx + CURR_CELL + 1*8], mm0

		pfmul          mm0, Downsampler2x3dnow_half
		pfacc          mm0, mm0
		movd           result, mm0

		femms
	}

	return result;
}



/*
==============================================================================
Name: process_block
Description:
	Downsamples (x2) a block of samples.
	Input and output blocks may overlap, see assert() for details.
Input parameters:
	- in_ptr: Input array, containing nbr_spl * 2 samples.
	- nbr_spl: Number of samples to output, > 0
Output parameters:
	- out_ptr: Array for the output samples, capacity: nbr_spl samples.
Throws: Nothing
==============================================================================
*/

template <int NC>
void	Downsampler2x3dnow <NC>::process_block (float out_ptr [], const float in_ptr [], long nbr_spl) noexcept
{
	assert (in_ptr  != nullptr);
	assert (out_ptr != nullptr);
	assert (out_ptr <= in_ptr || out_ptr >= in_ptr + nbr_spl * 2);
	assert (nbr_spl > 0);

	constexpr int  CURR_CELL = _nbr_stages * sizeof (_filter [0]);

	StageData3dnow *	filter_ptr = &_filter [0];

	__asm
	{
		mov            esi, in_ptr
		mov            edi, out_ptr
		mov            eax, nbr_spl
		mov            edx, filter_ptr
		lea            esi, [esi + eax*8]
		lea            edi, [edi + eax*4 - 4]
		neg            eax

	loop_sample:

		movq           mm0, [esi + eax*8]
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
		movq           [edx + CURR_CELL + 1*8], mm0

		inc            eax
		pfmul          mm0, Downsampler2x3dnow_half
		pfacc          mm0, mm0
		movd           [edi + eax*4], mm0

		jl             loop_sample

		femms
	}
}

// We could write the same specialisation for <7>
template <>
void	Downsampler2x3dnow <8>::process_block (float out_ptr [], const float in_ptr [], long nbr_spl) noexcept
{
	StageData3dnow *  filter_ptr = &_filter [0];

	__asm
	{
		mov            esi, in_ptr
		mov            edi, out_ptr
		mov            eax, nbr_spl
		lea            esi, [esi + eax*8]
		lea            edi, [edi + eax*4 - 4]
		neg            eax
		mov            edx, filter_ptr

		movq           mm2, [edx + 0*16 + 1*8]
		movq           mm3, [edx + 1*16 + 1*8]
		movq           mm4, [edx + 2*16 + 1*8]
		movq           mm5, [edx + 3*16 + 1*8]
		movq           mm6, [edx + 4*16 + 1*8]

	loop_sample:

		movq           mm0, [esi + eax*8]

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

		pfmul          mm0, Downsampler2x3dnow_half
		pfacc          mm0, mm0
		movd           [edi + eax*4], mm0

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
Name: process_sample_split
Description:
	Split (spectrum-wise) in half a pair of samples. The lower part of the
	spectrum is a classic downsampling, equivalent to the output of
	process_sample().
	The higher part is the complementary signal: original filter response
	is flipped from left to right, becoming a high-pass filter with the same
	cutoff frequency. This signal is then critically sampled (decimation by 2),
	flipping the spectrum: Fs/4...Fs/2 becomes Fs/4...0.
Input parameters:
	- in_ptr: pointer on the pair of input samples
Output parameters:
	- low: output sample, lower part of the spectrum (downsampling)
	- high: output sample, higher part of the spectrum.
Throws: Nothing
==============================================================================
*/

template <int NC>
void	Downsampler2x3dnow <NC>::process_sample_split (float &low, float &high, const float in_ptr [2]) noexcept
{
	assert (in_ptr != nullptr);

	constexpr int  CURR_CELL = _nbr_stages * sizeof (_filter [0]);

	StageData3dnow *  filter_ptr = &_filter [0];

	__asm
	{
		mov            esi, in_ptr
		mov            edx, filter_ptr
		mov            edi, low
		movq           mm0, [esi]
	}
	StageProc3dnow <_nbr_stages>::process_sample_pos ();
	__asm
	{
		movq           [edx + CURR_CELL + 1*8], mm0

		movq           mm1, mm0
		pfmul          mm0, Downsampler2x3dnow_half
		mov            ecx, high
		pfacc          mm0, mm0
		movd           [edi], mm0
		pfsub          mm1, mm0
		movd           [ecx], mm1

		femms
	}
	// Result storing could be optimized on Athlon with:
	// pfmul + pfpnacc + movd + punpckhdq + movd (not tested)
}



/*
==============================================================================
Name: process_block_split
Description:
	Split (spectrum-wise) in half a block of samples. The lower part of the
	spectrum is a classic downsampling, equivalent to the output of
	process_block().
	The higher part is the complementary signal: original filter response
	is flipped from left to right, becoming a high-pass filter with the same
	cutoff frequency. This signal is then critically sampled (decimation by 2),
	flipping the spectrum: Fs/4...Fs/2 becomes Fs/4...0.
	Input and output blocks may overlap, see assert() for details.
Input parameters:
	- in_ptr: Input array, containing nbr_spl * 2 samples.
	- nbr_spl: Number of samples for each output, > 0
Output parameters:
	- out_l_ptr: Array for the output samples, lower part of the spectrum
		(downsampling). Capacity: nbr_spl samples.
	- out_h_ptr: Array for the output samples, higher part of the spectrum.
		Capacity: nbr_spl samples.
Throws: Nothing
==============================================================================
*/

template <int NC>
void	Downsampler2x3dnow <NC>::process_block_split (float out_l_ptr [], float out_h_ptr [], const float in_ptr [], long nbr_spl) noexcept
{
	assert (in_ptr    != nullptr);
	assert (out_l_ptr != nullptr);
	assert (out_l_ptr <= in_ptr || out_l_ptr >= in_ptr + nbr_spl * 2);
	assert (out_h_ptr != nullptr);
	assert (out_h_ptr <= in_ptr || out_h_ptr >= in_ptr + nbr_spl * 2);
	assert (out_h_ptr != out_l_ptr);
	assert (nbr_spl > 0);

	constexpr int  CURR_CELL = _nbr_stages * sizeof (_filter [0]);

	StageData3dnow *  filter_ptr = &_filter [0];

	__asm
	{
		mov            esi, in_ptr
		mov            edi, out_l_ptr
		mov            ecx, out_h_ptr
		mov            eax, nbr_spl
		mov            edx, filter_ptr
		lea            esi, [esi + eax*8]
		lea            edi, [edi + eax*4 - 4]
		lea            ecx, [ecx + eax*4 - 4]
		neg            eax

	loop_sample:

		movq           mm0, [esi + eax*8]
	}
#if defined (_MSC_VER) && ! defined (NDEBUG)
	__asm push        eax
	__asm push        ecx
#endif
	StageProc3dnow <_nbr_stages>::process_sample_pos ();
#if defined (_MSC_VER) && ! defined (NDEBUG)
	__asm pop         ecx
	__asm pop         eax
#endif
	__asm
	{
		movq           [edx + CURR_CELL + 1*8], mm0

		movq           mm1, mm0
		inc            eax
		pfmul          mm0, Downsampler2x3dnow_half
		pfacc          mm0, mm0
		movd           [edi + eax*4], mm0
		pfsub          mm1, mm0
		movd           [ecx + eax*4], mm1

		jl             loop_sample

		femms
	}
	// Result storing could be optimized on Athlon with:
	// pfmul + pfpnacc + movd + punpckhdq + movd (not tested)
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
void	Downsampler2x3dnow <NC>::clear_buffers () noexcept
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
constexpr int	Downsampler2x3dnow <NC>::_stage_width;
template <int NC>
constexpr int	Downsampler2x3dnow <NC>::_nbr_stages;



}  // namespace hiir



#if defined (_MSC_VER)
#pragma warning (pop)
#endif



#endif   // hiir_Downsampler2x3dnow_CODEHEADER_INCLUDED

#undef hiir_Downsampler2x3dnow_CURRENT_CODEHEADER



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
