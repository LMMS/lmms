/*****************************************************************************

        StageProcSseV2.h
        Author: Laurent de Soras, 2020

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (hiir_StageProcSseV2_HEADER_INCLUDED)
#define hiir_StageProcSseV2_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/def.h"

#include <xmmintrin.h>



namespace hiir
{



class StageDataSse;

template <int CUR>
class StageProcSseV2
{

	static_assert ((CUR >= 0), "CUR must be >= 0");

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	static hiir_FORCEINLINE void
	               process_sample_pos (__m128 &x, StageDataSse *stage_arr) noexcept;
	static hiir_FORCEINLINE void
	               process_sample_neg (__m128 &x, StageDataSse *stage_arr) noexcept;

	static hiir_FORCEINLINE void
	               process_sample_pos_rec (__m128 &x, StageDataSse *stage_arr) noexcept;
	static hiir_FORCEINLINE void
	               process_sample_neg_rec (__m128 &x, StageDataSse *stage_arr) noexcept;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               StageProcSseV2 ()                               = delete;
	               StageProcSseV2 (const StageProcSseV2 &other)  = delete;
	               StageProcSseV2 (StageProcSseV2 &&other)       = delete;
	               ~StageProcSseV2 ()                              = delete;
	StageProcSseV2 &
	               operator = (const StageProcSseV2 &other)        = delete;
	StageProcSseV2 &
	               operator = (StageProcSseV2 &&other)             = delete;
	bool           operator == (const StageProcSseV2 &other) const = delete;
	bool           operator != (const StageProcSseV2 &other) const = delete;

}; // class StageProcSseV2



}  // namespace hiir



#include "hiir/StageProcSseV2.hpp"



#endif   // hiir_StageProcSseV2_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
