/*****************************************************************************

        StageProc4Neon.h
        Author: Laurent de Soras, 2016

Template parameters:
	- REMAINING: Number of remaining coefficients to process, >= 0

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (hiir_StageProc4Neon_HEADER_INCLUDED)
#define hiir_StageProc4Neon_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/def.h"



namespace hiir
{



class StageDataNeonV4;

template <int REMAINING>
class StageProc4Neon
{
	static_assert (REMAINING >= 0, "REMAINING must be >= 0.");

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	static hiir_FORCEINLINE void
	               process_sample_pos (const int nbr_coefs, float32x4_t &spl_0, float32x4_t &spl_1, StageDataNeonV4 *stage_arr) noexcept;
	static hiir_FORCEINLINE void
	               process_sample_neg (const int nbr_coefs, float32x4_t &spl_0, float32x4_t &spl_1, StageDataNeonV4 *stage_arr) noexcept;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               StageProc4Neon ()                                        = delete;
	               StageProc4Neon (const StageProc4Neon <REMAINING> &other) = delete;
	               StageProc4Neon (StageProc4Neon <REMAINING> &&other)      = delete;
	               ~StageProc4Neon ()                                       = delete;
	StageProc4Neon <REMAINING> &
	               operator = (const StageProc4Neon <REMAINING> &other)     = delete;
	StageProc4Neon <REMAINING> &
	               operator = (StageProc4Neon <REMAINING> &&other)          = delete;
	bool           operator == (const StageProc4Neon <REMAINING> &other) const = delete;
	bool           operator != (const StageProc4Neon <REMAINING> &other) const = delete;

}; // class StageProc4Neon



}  // namespace hiir



#include "hiir/StageProc4Neon.hpp"



#endif   // hiir_StageProc4Neon_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
