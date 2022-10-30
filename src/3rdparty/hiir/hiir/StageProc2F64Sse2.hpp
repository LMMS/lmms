/*****************************************************************************

        StageProc2F64Sse2.hpp
        Port of StageProc4Sse.hpp from float to double by Dario Mambro
        StageProc4Sse.hpp Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if defined (hiir_StageProc2F64Sse2_CURRENT_CODEHEADER)
	#error Recursive inclusion of StageProc2F64Sse2 code header.
#endif
#define hiir_StageProc2F64Sse2_CURRENT_CODEHEADER

#if ! defined (hiir_StageProc2F64Sse2_CODEHEADER_INCLUDED)
#define hiir_StageProc2F64Sse2_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/StageDataF64Sse2.h"



namespace hiir
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <>
hiir_FORCEINLINE void	StageProc2F64Sse2 <1>::process_sample_pos (const int nbr_coefs, __m128d &spl_0, __m128d &spl_1, StageDataF64Sse2 *stage_arr) noexcept
{
	const int      cnt   = nbr_coefs + 2 - 1;

	const __m128d   tmp_0 = _mm_add_pd (
		_mm_mul_pd (
			_mm_sub_pd (spl_0, _mm_load_pd (stage_arr [cnt    ]._mem)),
			_mm_load_pd (stage_arr [cnt    ]._coef)
		),
		_mm_load_pd (stage_arr [cnt - 2]._mem)
	);

	_mm_store_pd (stage_arr [cnt - 2]._mem, spl_0);
	_mm_store_pd (stage_arr [cnt - 1]._mem, spl_1);
	_mm_store_pd (stage_arr [cnt    ]._mem, tmp_0);

	spl_0 = tmp_0;
}



template <>
hiir_FORCEINLINE void	StageProc2F64Sse2 <0>::process_sample_pos (const int nbr_coefs, __m128d &spl_0, __m128d &spl_1, StageDataF64Sse2 *stage_arr) noexcept
{
	const int      cnt = nbr_coefs + 2;

	_mm_store_pd (stage_arr [cnt - 2]._mem, spl_0);
	_mm_store_pd (stage_arr [cnt - 1]._mem, spl_1);
}



template <int REMAINING>
void	StageProc2F64Sse2 <REMAINING>::process_sample_pos (const int nbr_coefs, __m128d &spl_0, __m128d &spl_1, StageDataF64Sse2 *stage_arr) noexcept
{
	const int      cnt   = nbr_coefs + 2 - REMAINING;

	const __m128d   tmp_0 = _mm_add_pd (
		_mm_mul_pd (
			_mm_sub_pd (spl_0, _mm_load_pd (stage_arr [cnt    ]._mem)),
			_mm_load_pd (stage_arr [cnt    ]._coef)
		),
		_mm_load_pd (stage_arr [cnt - 2]._mem)
	);
	const __m128d   tmp_1 = _mm_add_pd (
		_mm_mul_pd (
			_mm_sub_pd (spl_1, _mm_load_pd (stage_arr [cnt + 1]._mem)),
			_mm_load_pd (stage_arr [cnt + 1]._coef)
		),
		_mm_load_pd (stage_arr [cnt - 1]._mem)
	);

	_mm_store_pd (stage_arr [cnt - 2]._mem, spl_0);
	_mm_store_pd (stage_arr [cnt - 1]._mem, spl_1);

	spl_0 = tmp_0;
	spl_1 = tmp_1;

	StageProc2F64Sse2 <REMAINING - 2>::process_sample_pos (
		nbr_coefs,
		spl_0,
		spl_1,
		stage_arr
	);
}



template <>
hiir_FORCEINLINE void	StageProc2F64Sse2 <1>::process_sample_neg (const int nbr_coefs, __m128d &spl_0, __m128d &spl_1, StageDataF64Sse2 *stage_arr) noexcept
{
	const int      cnt   = nbr_coefs + 2 - 1;

	const __m128d   tmp_0 = _mm_sub_pd (
		_mm_mul_pd (
			_mm_add_pd (spl_0, _mm_load_pd (stage_arr [cnt    ]._mem)),
			_mm_load_pd (stage_arr [cnt    ]._coef)
		),
		_mm_load_pd (stage_arr [cnt - 2]._mem)
	);

	_mm_store_pd (stage_arr [cnt - 2]._mem, spl_0);
	_mm_store_pd (stage_arr [cnt - 1]._mem, spl_1);
	_mm_store_pd (stage_arr [cnt    ]._mem, tmp_0);

	spl_0 = tmp_0;
}



template <>
hiir_FORCEINLINE void	StageProc2F64Sse2 <0>::process_sample_neg (const int nbr_coefs, __m128d &spl_0, __m128d &spl_1, StageDataF64Sse2 *stage_arr) noexcept
{
	const int      cnt = nbr_coefs + 2;

	_mm_store_pd (stage_arr [cnt - 2]._mem, spl_0);
	_mm_store_pd (stage_arr [cnt - 1]._mem, spl_1);
}



template <int REMAINING>
void	StageProc2F64Sse2 <REMAINING>::process_sample_neg (const int nbr_coefs, __m128d &spl_0, __m128d &spl_1, StageDataF64Sse2 *stage_arr) noexcept
{
	const int      cnt   = nbr_coefs + 2 - REMAINING;

	const __m128d   tmp_0 = _mm_sub_pd (
		_mm_mul_pd (
			_mm_add_pd (spl_0, _mm_load_pd (stage_arr [cnt    ]._mem)),
			_mm_load_pd (stage_arr [cnt    ]._coef)
		),
		_mm_load_pd (stage_arr [cnt - 2]._mem)
	);
	const __m128d   tmp_1 = _mm_sub_pd (
		_mm_mul_pd (
			_mm_add_pd (spl_1, _mm_load_pd (stage_arr [cnt + 1]._mem)),
			_mm_load_pd (stage_arr [cnt + 1]._coef)
		),
		_mm_load_pd (stage_arr [cnt - 1]._mem)
	);

	_mm_store_pd (stage_arr [cnt - 2]._mem, spl_0);
	_mm_store_pd (stage_arr [cnt - 1]._mem, spl_1);

	spl_0 = tmp_0;
	spl_1 = tmp_1;

	StageProc2F64Sse2 <REMAINING - 2>::process_sample_neg (
		nbr_coefs,
		spl_0,
		spl_1,
		stage_arr
	);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}  // namespace hiir



#endif   // hiir_StageProc2F64Sse2_CODEHEADER_INCLUDED

#undef hiir_StageProc2F64Sse2_CURRENT_CODEHEADER



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
