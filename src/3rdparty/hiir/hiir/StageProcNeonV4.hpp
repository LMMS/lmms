/*****************************************************************************

        StageProcNeonV4.hpp
        Author: Laurent de Soras, 2016

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (hiir_StageProcNeonV4_CODEHEADER_INCLUDED)
#define hiir_StageProcNeonV4_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/fnc_neon.h"
#include "hiir/StageDataNeonV4.h"



namespace hiir
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int CUR>
void	StageProcNeonV4 <CUR>::process_sample_pos (StageDataNeonV4 *stage_ptr, float32x4_t &y, float32x4_t &mem) noexcept
{
	StageProcNeonV4 <CUR>::process_sample_pos_rec (stage_ptr, y, mem);
	storea (stage_ptr [CUR]._mem, y);
}



template <int CUR>
void	StageProcNeonV4 <CUR>::process_sample_neg (StageDataNeonV4 *stage_ptr, float32x4_t &y, float32x4_t &mem) noexcept
{
	StageProcNeonV4 <CUR>::process_sample_neg_rec (stage_ptr, y, mem);
	storea (stage_ptr [CUR]._mem, y);
}



template <int CUR>
void	StageProcNeonV4 <CUR>::process_sample_pos_rec (StageDataNeonV4 *stage_ptr, float32x4_t &y, float32x4_t &mem) noexcept
{
	StageProcNeonV4 <CUR - 1>::process_sample_pos_rec (stage_ptr, y, mem);

	const float32x4_t x    = mem;
	storea (stage_ptr [CUR - 1]._mem, y);

	mem = load4a (stage_ptr [CUR]._mem);
	y   = vmlaq_f32 (x, y - mem, load4a (stage_ptr [CUR]._coef));
}

template <>
hiir_FORCEINLINE void	StageProcNeonV4 <0>::process_sample_pos_rec (StageDataNeonV4 * /* stage_ptr */, float32x4_t & /* y */, float32x4_t & /* mem */) noexcept
{
	// Nothing, stops the recursion
}



template <int CUR>
void	StageProcNeonV4 <CUR>::process_sample_neg_rec (StageDataNeonV4 *stage_ptr, float32x4_t &y, float32x4_t &mem) noexcept
{
	StageProcNeonV4 <CUR - 1>::process_sample_neg_rec (stage_ptr, y, mem);

	const float32x4_t x = mem;
	storea (stage_ptr [CUR - 1]._mem, y);

	mem = load4a (stage_ptr [CUR]._mem);
	y  += mem;
	y  *= load4a (stage_ptr [CUR]._coef);
	y  -= x;
}

template <>
hiir_FORCEINLINE void	StageProcNeonV4 <0>::process_sample_neg_rec (StageDataNeonV4 * /* stage_ptr */, float32x4_t & /* y */, float32x4_t & /* mem */) noexcept
{
	// Nothing, stops the recursion
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}  // namespace hiir



#endif   // hiir_StageProcNeonV4_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
