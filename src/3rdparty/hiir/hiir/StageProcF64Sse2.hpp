/*****************************************************************************

        StageProcF64Sse2.hpp
        Author: Laurent de Soras, 2020

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#if ! defined (hiir_StageProcF64Sse2_CODEHEADER_INCLUDED)
#define hiir_StageProcF64Sse2_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



namespace hiir
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int CUR>
void  StageProcF64Sse2 <CUR>::process_sample_pos (__m128d &x, StageDataF64Sse2 *stage_arr) noexcept
{
  StageProcF64Sse2 <CUR>::process_sample_pos_rec (x, stage_arr);
	_mm_store_pd (stage_arr [CUR]._mem, x);
}



template <int CUR>
void  StageProcF64Sse2 <CUR>::process_sample_neg (__m128d &x, StageDataF64Sse2 *stage_arr) noexcept
{
  StageProcF64Sse2 <CUR>::process_sample_neg_rec (x, stage_arr);
	_mm_store_pd (stage_arr [CUR]._mem, x);
}



template <int CUR>
void  StageProcF64Sse2 <CUR>::process_sample_pos_rec (__m128d &x, StageDataF64Sse2 *stage_arr) noexcept
{
   StageProcF64Sse2 <CUR - 1>::process_sample_pos_rec (x, stage_arr);

   const auto     tmp = _mm_load_pd (stage_arr [CUR - 1]._mem);
   _mm_store_pd (stage_arr [CUR - 1]._mem, x);

   x = _mm_sub_pd (x, _mm_load_pd (stage_arr [CUR]._mem ));
   x = _mm_mul_pd (x, _mm_load_pd (stage_arr [CUR]._coef));
   x = _mm_add_pd (x, tmp);
}

template <>
hiir_FORCEINLINE void  StageProcF64Sse2 <0>::process_sample_pos_rec (__m128d & /*x*/, StageDataF64Sse2 * /*stage_arr*/) noexcept
{
	// Nothing, stops the recursion
}



template <int CUR>
void  StageProcF64Sse2 <CUR>::process_sample_neg_rec (__m128d &x, StageDataF64Sse2 *stage_arr) noexcept
{
   StageProcF64Sse2 <CUR - 1>::process_sample_neg_rec (x, stage_arr);

   const auto     tmp = _mm_load_pd (stage_arr [CUR - 1]._mem);
   _mm_store_pd (stage_arr [CUR - 1]._mem, x);

   x = _mm_add_pd (x, _mm_load_pd (stage_arr [CUR]._mem ));
   x = _mm_mul_pd (x, _mm_load_pd (stage_arr [CUR]._coef));
   x = _mm_sub_pd (x, tmp);
}

template <>
hiir_FORCEINLINE void  StageProcF64Sse2 <0>::process_sample_neg_rec (__m128d & /*x*/, StageDataF64Sse2 * /*stage_arr*/) noexcept
{
	// Nothing, stops the recursion
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}  // namespace hiir



#endif   // hiir_StageProcF64Sse2_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
