/*****************************************************************************

        StageProc16Avx512.hpp
        Author: Laurent de Soras, 2020

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if defined (hiir_StageProc16Avx512_CURRENT_CODEHEADER)
	#error Recursive inclusion of StageProc16Avx512 code header.
#endif
#define hiir_StageProc16Avx512_CURRENT_CODEHEADER

#if ! defined (hiir_StageProc16Avx512_CODEHEADER_INCLUDED)
#define hiir_StageProc16Avx512_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/StageDataAvx512.h"



namespace hiir
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <>
hiir_FORCEINLINE void	StageProc16Avx512 <1>::process_sample_pos (const int nbr_coefs, __m512 &spl_0, __m512 &spl_1, StageDataAvx512 *stage_arr) noexcept
{
	const int      cnt   = nbr_coefs + 2 - 1;

	const __m512   tmp_0 = _mm512_add_ps (
		_mm512_mul_ps (
			_mm512_sub_ps (spl_0, _mm512_load_ps (stage_arr [cnt    ]._mem)),
			_mm512_load_ps (stage_arr [cnt    ]._coef)
		),
		_mm512_load_ps (stage_arr [cnt - 2]._mem)
	);

	_mm512_store_ps (stage_arr [cnt - 2]._mem, spl_0);
	_mm512_store_ps (stage_arr [cnt - 1]._mem, spl_1);
	_mm512_store_ps (stage_arr [cnt    ]._mem, tmp_0);

	spl_0 = tmp_0;
}



template <>
hiir_FORCEINLINE void	StageProc16Avx512 <0>::process_sample_pos (const int nbr_coefs, __m512 &spl_0, __m512 &spl_1, StageDataAvx512 *stage_arr) noexcept
{
	const int      cnt = nbr_coefs + 2;

	_mm512_store_ps (stage_arr [cnt - 2]._mem, spl_0);
	_mm512_store_ps (stage_arr [cnt - 1]._mem, spl_1);
}



template <int REMAINING>
void	StageProc16Avx512 <REMAINING>::process_sample_pos (const int nbr_coefs, __m512 &spl_0, __m512 &spl_1, StageDataAvx512 *stage_arr) noexcept
{
	const int      cnt   = nbr_coefs + 2 - REMAINING;

	const __m512   tmp_0 = _mm512_add_ps (
		_mm512_mul_ps (
			_mm512_sub_ps (spl_0, _mm512_load_ps (stage_arr [cnt    ]._mem)),
			_mm512_load_ps (stage_arr [cnt    ]._coef)
		),
		_mm512_load_ps (stage_arr [cnt - 2]._mem)
	);
	const __m512   tmp_1 = _mm512_add_ps (
		_mm512_mul_ps (
			_mm512_sub_ps (spl_1, _mm512_load_ps (stage_arr [cnt + 1]._mem)),
			_mm512_load_ps (stage_arr [cnt + 1]._coef)
		),
		_mm512_load_ps (stage_arr [cnt - 1]._mem)
	);

	_mm512_store_ps (stage_arr [cnt - 2]._mem, spl_0);
	_mm512_store_ps (stage_arr [cnt - 1]._mem, spl_1);

	spl_0 = tmp_0;
	spl_1 = tmp_1;

	StageProc16Avx512 <REMAINING - 2>::process_sample_pos (
		nbr_coefs,
		spl_0,
		spl_1,
		stage_arr
	);
}



template <>
hiir_FORCEINLINE void	StageProc16Avx512 <1>::process_sample_neg (const int nbr_coefs, __m512 &spl_0, __m512 &spl_1, StageDataAvx512 *stage_arr) noexcept
{
	const int      cnt   = nbr_coefs + 2 - 1;

	const __m512   tmp_0 = _mm512_sub_ps (
		_mm512_mul_ps (
			_mm512_add_ps (spl_0, _mm512_load_ps (stage_arr [cnt    ]._mem)),
			_mm512_load_ps (stage_arr [cnt    ]._coef)
		),
		_mm512_load_ps (stage_arr [cnt - 2]._mem)
	);

	_mm512_store_ps (stage_arr [cnt - 2]._mem, spl_0);
	_mm512_store_ps (stage_arr [cnt - 1]._mem, spl_1);
	_mm512_store_ps (stage_arr [cnt    ]._mem, tmp_0);

	spl_0 = tmp_0;
}



template <>
hiir_FORCEINLINE void	StageProc16Avx512 <0>::process_sample_neg (const int nbr_coefs, __m512 &spl_0, __m512 &spl_1, StageDataAvx512 *stage_arr) noexcept
{
	const int      cnt = nbr_coefs + 2;

	_mm512_store_ps (stage_arr [cnt - 2]._mem, spl_0);
	_mm512_store_ps (stage_arr [cnt - 1]._mem, spl_1);
}



template <int REMAINING>
void	StageProc16Avx512 <REMAINING>::process_sample_neg (const int nbr_coefs, __m512 &spl_0, __m512 &spl_1, StageDataAvx512 *stage_arr) noexcept
{
	const int      cnt   = nbr_coefs + 2 - REMAINING;

	const __m512   tmp_0 = _mm512_sub_ps (
		_mm512_mul_ps (
			_mm512_add_ps (spl_0, _mm512_load_ps (stage_arr [cnt    ]._mem)),
			_mm512_load_ps (stage_arr [cnt    ]._coef)
		),
		_mm512_load_ps (stage_arr [cnt - 2]._mem)
	);
	const __m512   tmp_1 = _mm512_sub_ps (
		_mm512_mul_ps (
			_mm512_add_ps (spl_1, _mm512_load_ps (stage_arr [cnt + 1]._mem)),
			_mm512_load_ps (stage_arr [cnt + 1]._coef)
		),
		_mm512_load_ps (stage_arr [cnt - 1]._mem)
	);

	_mm512_store_ps (stage_arr [cnt - 2]._mem, spl_0);
	_mm512_store_ps (stage_arr [cnt - 1]._mem, spl_1);

	spl_0 = tmp_0;
	spl_1 = tmp_1;

	StageProc16Avx512 <REMAINING - 2>::process_sample_neg (
		nbr_coefs,
		spl_0,
		spl_1,
		stage_arr
	);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}  // namespace hiir



#endif   // hiir_StageProc16Avx512_CODEHEADER_INCLUDED

#undef hiir_StageProc16Avx512_CURRENT_CODEHEADER



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
