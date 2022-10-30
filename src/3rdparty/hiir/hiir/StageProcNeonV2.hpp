/*****************************************************************************

        StageProcNeonV2.hpp
        Author: Laurent de Soras, 2016

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (hiir_StageProcNeonV2_CODEHEADER_INCLUDED)
#define hiir_StageProcNeonV2_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/fnc_neon.h"
#include "hiir/StageDataNeonV2.h"



namespace hiir
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int CUR>
void	StageProcNeonV2 <CUR>::process_sample_pos (float32x2_t &x, StageDataNeonV2 *stage_ptr) noexcept
{
	StageProcNeonV2 <CUR>::process_sample_pos_rec (x, stage_ptr);
	storea (stage_ptr [CUR]._mem, x);
}



template <int CUR>
void	StageProcNeonV2 <CUR>::process_sample_neg (float32x2_t &x, StageDataNeonV2 *stage_ptr) noexcept
{
	StageProcNeonV2 <CUR>::process_sample_neg_rec (x, stage_ptr);
	storea (stage_ptr [CUR]._mem, x);
}



template <int CUR>
void	StageProcNeonV2 <CUR>::process_sample_pos_rec (float32x2_t &x, StageDataNeonV2 *stage_ptr) noexcept
{
	StageProcNeonV2 <CUR - 1>::process_sample_pos_rec (x, stage_ptr);

	const float32x2_t tmp = load2a (stage_ptr [CUR - 1]._mem);
	storea (stage_ptr [CUR - 1]._mem, x);

	x = vmla_f32 (
		tmp,
		x - load2a (stage_ptr [CUR]._mem),
		load2a (stage_ptr [CUR]._coef)
	);
}

template <>
hiir_FORCEINLINE void	StageProcNeonV2 <0>::process_sample_pos_rec (float32x2_t &/* x */, StageDataNeonV2 * /* stage_ptr */) noexcept
{
	// Nothing, stops the recursion
}



template <int CUR>
void	StageProcNeonV2 <CUR>::process_sample_neg_rec (float32x2_t &x, StageDataNeonV2 *stage_ptr) noexcept
{
	StageProcNeonV2 <CUR - 1>::process_sample_neg_rec (x, stage_ptr);

	const float32x2_t tmp = load2a (stage_ptr [CUR - 1]._mem);
	storea (stage_ptr [CUR - 1]._mem, x);

	x += load2a (stage_ptr [CUR]._mem );
	x *= load2a (stage_ptr [CUR]._coef);
	x -= tmp;
}

template <>
hiir_FORCEINLINE void	StageProcNeonV2 <0>::process_sample_neg_rec (float32x2_t &/* x */, StageDataNeonV2 * /* stage_ptr */) noexcept
{
	// Nothing, stops the recursion
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}  // namespace hiir



#endif   // hiir_StageProcNeonV2_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
