/*****************************************************************************

        StageProc4Neon.hpp
        Author: Laurent de Soras, 2016

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (hiir_StageProc4Neon_CODEHEADER_INCLUDED)
#define hiir_StageProc4Neon_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/StageDataNeonV4.h"



namespace hiir
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <>
inline void	StageProc4Neon <1>::process_sample_pos (const int nbr_coefs, float32x4_t &spl_0, float32x4_t &spl_1, StageDataNeonV4 *stage_arr) noexcept
{
	const int      cnt = nbr_coefs + 2 - 1;

	const float32x4_t tmp_0 = vmlaq_f32 (
		        load4a (stage_arr [cnt - 2]._mem ),
		spl_0 - load4a (stage_arr [cnt    ]._mem ),
		        load4a (stage_arr [cnt    ]._coef)
	);

	storea (stage_arr [cnt - 2]._mem, spl_0);
	storea (stage_arr [cnt - 1]._mem, spl_1);
	storea (stage_arr [cnt    ]._mem, tmp_0);

	spl_0 = tmp_0;
}



template <>
inline void	StageProc4Neon <0>::process_sample_pos (const int nbr_coefs, float32x4_t &spl_0, float32x4_t &spl_1, StageDataNeonV4 *stage_arr) noexcept
{
	const int      cnt = nbr_coefs + 2;

	storea (stage_arr [cnt - 2]._mem, spl_0);
	storea (stage_arr [cnt - 1]._mem, spl_1);
}



template <int REMAINING>
void	StageProc4Neon <REMAINING>::process_sample_pos (const int nbr_coefs, float32x4_t &spl_0, float32x4_t &spl_1, StageDataNeonV4 *stage_arr) noexcept
{
	const int      cnt = nbr_coefs + 2 - REMAINING;

	const float32x4_t tmp_0 = vmlaq_f32 (
		        load4a (stage_arr [cnt - 2]._mem ),
		spl_0 - load4a (stage_arr [cnt    ]._mem ),
		        load4a (stage_arr [cnt    ]._coef)
	);
	const float32x4_t tmp_1 = vmlaq_f32 (
		        load4a (stage_arr [cnt - 1]._mem ),
		spl_1 - load4a (stage_arr [cnt + 1]._mem ),
		        load4a (stage_arr [cnt + 1]._coef)
	);

	storea (stage_arr [cnt - 2]._mem, spl_0);
	storea (stage_arr [cnt - 1]._mem, spl_1);

	spl_0 = tmp_0;
	spl_1 = tmp_1;

	StageProc4Neon <REMAINING - 2>::process_sample_pos (
		nbr_coefs,
		spl_0,
		spl_1,
		stage_arr
	);
}



template <>
inline void	StageProc4Neon <1>::process_sample_neg (const int nbr_coefs, float32x4_t &spl_0, float32x4_t &spl_1, StageDataNeonV4 *stage_arr) noexcept
{
	const int      cnt = nbr_coefs + 2 - 1;

	float32x4_t tmp_0 = spl_0;
	tmp_0 += load4a (stage_arr [cnt    ]._mem );
	tmp_0 *= load4a (stage_arr [cnt    ]._coef);
	tmp_0 -= load4a (stage_arr [cnt - 2]._mem );

	storea (stage_arr [cnt - 2]._mem, spl_0);
	storea (stage_arr [cnt - 1]._mem, spl_1);
	storea (stage_arr [cnt    ]._mem, tmp_0);

	spl_0 = tmp_0;
}

template <>
inline void	StageProc4Neon <0>::process_sample_neg (const int nbr_coefs, float32x4_t &spl_0, float32x4_t &spl_1, StageDataNeonV4 *stage_arr) noexcept
{
	const int      cnt = nbr_coefs + 2;

	storea (stage_arr [cnt - 2]._mem, spl_0);
	storea (stage_arr [cnt - 1]._mem, spl_1);
}

template <int REMAINING>
void	StageProc4Neon <REMAINING>::process_sample_neg (const int nbr_coefs, float32x4_t &spl_0, float32x4_t &spl_1, StageDataNeonV4 *stage_arr) noexcept
{
	const int      cnt = nbr_coefs + 2 - REMAINING;

	float32x4_t tmp_0 = spl_0;
	tmp_0 += load4a (stage_arr [cnt    ]._mem );
	tmp_0 *= load4a (stage_arr [cnt    ]._coef);
	tmp_0 -= load4a (stage_arr [cnt - 2]._mem );

	float32x4_t tmp_1 = spl_1;
	tmp_1 += load4a (stage_arr [cnt + 1]._mem );
	tmp_1 *= load4a (stage_arr [cnt + 1]._coef);
	tmp_1 -= load4a (stage_arr [cnt - 1]._mem );

	storea (stage_arr [cnt - 2]._mem, spl_0);
	storea (stage_arr [cnt - 1]._mem, spl_1);

	spl_0 = tmp_0;
	spl_1 = tmp_1;

	StageProc4Neon <REMAINING - 2>::process_sample_neg (
		nbr_coefs,
		spl_0,
		spl_1,
		stage_arr
	);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}  // namespace hiir



#endif   // hiir_StageProc4Neon_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
