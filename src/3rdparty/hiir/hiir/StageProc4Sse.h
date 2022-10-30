/*****************************************************************************

        StageProc4Sse.h
        Author: Laurent de Soras, 2015

Template parameters:
	- REMAINING: Number of remaining coefficients to process, >= 0

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (hiir_StageProc4Sse_HEADER_INCLUDED)
#define hiir_StageProc4Sse_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250) // "Inherits via dominance."
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/def.h"

#include <xmmintrin.h>



namespace hiir
{



class StageDataSse;

template <int REMAINING>
class StageProc4Sse
{

	static_assert ((REMAINING >= 0), "REMAINING must be >= 0");

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	static hiir_FORCEINLINE void
	               process_sample_pos (const int nbr_coefs, __m128 &spl_0, __m128 &spl_1, StageDataSse *stage_arr) noexcept;
	static hiir_FORCEINLINE void
	               process_sample_neg (const int nbr_coefs, __m128 &spl_0, __m128 &spl_1, StageDataSse *stage_arr) noexcept;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               StageProc4Sse ()                                       = delete;
	               StageProc4Sse (const StageProc4Sse <REMAINING> &other) = delete;
	               StageProc4Sse (StageProc4Sse <REMAINING> &&other)    = delete;
	               ~StageProc4Sse ()                                    = delete;
	StageProc4Sse <REMAINING> &
						operator = (const StageProc4Sse <REMAINING> &other)  = delete;
	StageProc4Sse <REMAINING> &
						operator = (StageProc4Sse <REMAINING> &&other)       = delete;
	bool           operator == (const StageProc4Sse <REMAINING> &other) = delete;
	bool           operator != (const StageProc4Sse <REMAINING> &other) = delete;

}; // class StageProc4Sse



}  // namespace hiir



#include "hiir/StageProc4Sse.hpp"



#endif   // hiir_StageProc4Sse_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
