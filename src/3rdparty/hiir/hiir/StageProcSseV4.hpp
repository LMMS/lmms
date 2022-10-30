/*****************************************************************************

        StageProcSseV4.hpp
        Author: Laurent de Soras, 2005

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if defined (hiir_StageProcSseV4_CURRENT_CODEHEADER)
	#error Recursive inclusion of StageProcSseV4 code header.
#endif
#define hiir_StageProcSseV4_CURRENT_CODEHEADER

#if ! defined (hiir_StageProcSseV4_CODEHEADER_INCLUDED)
#define hiir_StageProcSseV4_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/StageDataSse.h"



namespace hiir
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int CUR>
void	StageProcSseV4 <CUR>::process_sample_pos (StageDataSse *stage_ptr, __m128 &y, __m128 &mem) noexcept
{
	StageProcSseV4 <CUR - 1>::process_sample_pos (stage_ptr, y, mem);

	const __m128   x    = mem;
	_mm_store_ps (stage_ptr [PREV]._mem, y);

	mem = _mm_load_ps (stage_ptr [CUR]._mem);
	y   = _mm_sub_ps (y, mem);
	const __m128   coef = _mm_load_ps (stage_ptr [CUR]._coef);
	y   = _mm_mul_ps (y, coef);
	y   = _mm_add_ps (y, x);
}

template <>
hiir_FORCEINLINE void	StageProcSseV4 <0>::process_sample_pos (StageDataSse * /* stage_ptr */, __m128 & /* y */, __m128 & /* mem */) noexcept
{
	// Nothing, stops the recursion
}



template <int CUR>
void	StageProcSseV4 <CUR>::process_sample_neg (StageDataSse *stage_ptr, __m128 &y, __m128 &mem) noexcept
{
	StageProcSseV4 <CUR - 1>::process_sample_neg (stage_ptr, y, mem);

	const __m128   x    = mem;
	_mm_store_ps (stage_ptr [PREV]._mem, y);

	mem = _mm_load_ps (stage_ptr [CUR]._mem);
	y   = _mm_add_ps (y, mem);
	const __m128   coef = _mm_load_ps (stage_ptr [CUR]._coef);
	y   = _mm_mul_ps (y, coef);
	y   = _mm_sub_ps (y, x);
}

template <>
hiir_FORCEINLINE void	StageProcSseV4 <0>::process_sample_neg (StageDataSse * /* stage_ptr */, __m128 & /* y */, __m128 & /* mem */) noexcept
{
	// Nothing, stops the recursion
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}  // namespace hiir



#endif   // hiir_StageProcSseV4_CODEHEADER_INCLUDED

#undef hiir_StageProcSseV4_CURRENT_CODEHEADER



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
