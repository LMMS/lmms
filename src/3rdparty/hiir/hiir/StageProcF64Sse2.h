/*****************************************************************************

        StageProcF64Sse2.h
        Author: Laurent de Soras, 2020

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (hiir_StageProcF64Sse2_HEADER_INCLUDED)
#define hiir_StageProcF64Sse2_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/def.h"

#include <emmintrin.h>



namespace hiir
{



class StageDataF64Sse2;

template <int CUR>
class StageProcF64Sse2
{

	static_assert ((CUR >= 0), "CUR must be >= 0");

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	static hiir_FORCEINLINE void
	               process_sample_pos (__m128d &x, StageDataF64Sse2 *stage_arr) noexcept;
	static hiir_FORCEINLINE void
	               process_sample_neg (__m128d &x, StageDataF64Sse2 *stage_arr) noexcept;

	static hiir_FORCEINLINE void
	               process_sample_pos_rec (__m128d &x, StageDataF64Sse2 *stage_arr) noexcept;
	static hiir_FORCEINLINE void
	               process_sample_neg_rec (__m128d &x, StageDataF64Sse2 *stage_arr) noexcept;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               StageProcF64Sse2 ()                               = delete;
	               StageProcF64Sse2 (const StageProcF64Sse2 &other)  = delete;
	               StageProcF64Sse2 (StageProcF64Sse2 &&other)       = delete;
	               ~StageProcF64Sse2 ()                              = delete;
	StageProcF64Sse2 &
	               operator = (const StageProcF64Sse2 &other)        = delete;
	StageProcF64Sse2 &
	               operator = (StageProcF64Sse2 &&other)             = delete;
	bool           operator == (const StageProcF64Sse2 &other) const = delete;
	bool           operator != (const StageProcF64Sse2 &other) const = delete;

}; // class StageProcF64Sse2



}  // namespace hiir



#include "hiir/StageProcF64Sse2.hpp"



#endif   // hiir_StageProcF64Sse2_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
