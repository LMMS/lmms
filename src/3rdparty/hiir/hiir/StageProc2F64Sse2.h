/*****************************************************************************

        StageProc2F64Sse2.h
        Port of StageProc4Sse.h from float to double by Dario Mambro
        StageProc4Sse.h Laurent de Soras, 2015

Template parameters:
	- REMAINING: Number of remaining coefficients to process, >= 0

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (hiir_StageProc2F64Sse2_HEADER_INCLUDED)
#define hiir_StageProc2F64Sse2_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250) // "Inherits via dominance."
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/def.h"

#include <emmintrin.h>



namespace hiir
{



class StageDataF64Sse2;

template <int REMAINING>
class StageProc2F64Sse2
{

	static_assert ((REMAINING >= 0), "REMAINING must be >= 0");

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	static hiir_FORCEINLINE void
	               process_sample_pos (const int nbr_coefs, __m128d &spl_0, __m128d &spl_1, StageDataF64Sse2 *stage_arr) noexcept;
	static hiir_FORCEINLINE void
	               process_sample_neg (const int nbr_coefs, __m128d &spl_0, __m128d &spl_1, StageDataF64Sse2 *stage_arr) noexcept;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               StageProc2F64Sse2 ()                                           = delete;
	               StageProc2F64Sse2 (const StageProc2F64Sse2 <REMAINING> &other) = delete;
	               StageProc2F64Sse2 (StageProc2F64Sse2 <REMAINING> &&other) = delete;
	               ~StageProc2F64Sse2 ()                                     = delete;
	StageProc2F64Sse2 <REMAINING> &
						operator = (const StageProc2F64Sse2 <REMAINING> &other)   = delete;
	StageProc2F64Sse2 <REMAINING> &
						operator = (StageProc2F64Sse2 <REMAINING> &&other)        = delete;
	bool           operator == (const StageProc2F64Sse2 <REMAINING> &other)  = delete;
	bool           operator != (const StageProc2F64Sse2 <REMAINING> &other)  = delete;

}; // class StageProc2F64Sse2



}  // namespace hiir



#include "hiir/StageProc2F64Sse2.hpp"



#endif   // hiir_StageProc2F64Sse2_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
