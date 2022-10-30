/*****************************************************************************

        StageProc16Avx512.h
        Author: Laurent de Soras, 2020

Template parameters:
	- REMAINING: Number of remaining coefficients to process, >= 0

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (hiir_StageProc16Avx512_HEADER_INCLUDED)
#define hiir_StageProc16Avx512_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250) // "Inherits via dominance."
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/def.h"

#include <immintrin.h>


namespace hiir
{



class StageDataAvx512;

template <int REMAINING>
class StageProc16Avx512
{

	static_assert ((REMAINING >= 0), "REMAINING must be >= 0");

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	static hiir_FORCEINLINE void
	               process_sample_pos (const int nbr_coefs, __m512 &spl_0, __m512 &spl_1, StageDataAvx512 *stage_arr) noexcept;
	static hiir_FORCEINLINE void
	               process_sample_neg (const int nbr_coefs, __m512 &spl_0, __m512 &spl_1, StageDataAvx512 *stage_arr) noexcept;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               StageProc16Avx512 ()                                       = delete;
	               StageProc16Avx512 (const StageProc16Avx512 <REMAINING> &other) = delete;
	               StageProc16Avx512 (StageProc16Avx512 <REMAINING> &&other)    = delete;
	               ~StageProc16Avx512 ()                                    = delete;
	StageProc16Avx512 <REMAINING> &
						operator = (const StageProc16Avx512 <REMAINING> &other)  = delete;
	StageProc16Avx512 <REMAINING> &
						operator = (StageProc16Avx512 <REMAINING> &&other)       = delete;
	bool           operator == (const StageProc16Avx512 <REMAINING> &other) = delete;
	bool           operator != (const StageProc16Avx512 <REMAINING> &other) = delete;

}; // class StageProc16Avx512



}  // namespace hiir



#include "hiir/StageProc16Avx512.hpp"



#endif   // hiir_StageProc16Avx512_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
