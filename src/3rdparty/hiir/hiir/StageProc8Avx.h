/*****************************************************************************

        StageProc8Avx.h
        Port of StageProc4See.h from SSE to AVX by Dario Mambro
        StageProc4See.h by Laurent de Soras

Template parameters:
	- REMAINING: Number of remaining coefficients to process, >= 0

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (hiir_StageProc8Avx_HEADER_INCLUDED)
#define hiir_StageProc8Avx_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250) // "Inherits via dominance."
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/def.h"

#include <immintrin.h>


namespace hiir
{



class StageDataAvx;

template <int REMAINING>
class StageProc8Avx
{

	static_assert ((REMAINING >= 0), "REMAINING must be >= 0");

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	static hiir_FORCEINLINE void
	               process_sample_pos (const int nbr_coefs, __m256 &spl_0, __m256 &spl_1, StageDataAvx *stage_arr) noexcept;
	static hiir_FORCEINLINE void
	               process_sample_neg (const int nbr_coefs, __m256 &spl_0, __m256 &spl_1, StageDataAvx *stage_arr) noexcept;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               StageProc8Avx ()                                       = delete;
	               StageProc8Avx (const StageProc8Avx <REMAINING> &other) = delete;
	               StageProc8Avx (StageProc8Avx <REMAINING> &&other)    = delete;
	               ~StageProc8Avx ()                                    = delete;
	StageProc8Avx <REMAINING> &
						operator = (const StageProc8Avx <REMAINING> &other)  = delete;
	StageProc8Avx <REMAINING> &
						operator = (StageProc8Avx <REMAINING> &&other)       = delete;
	bool           operator == (const StageProc8Avx <REMAINING> &other) = delete;
	bool           operator != (const StageProc8Avx <REMAINING> &other) = delete;

}; // class StageProc8Avx



}  // namespace hiir



#include "hiir/StageProc8Avx.hpp"



#endif   // hiir_StageProc8Avx_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
