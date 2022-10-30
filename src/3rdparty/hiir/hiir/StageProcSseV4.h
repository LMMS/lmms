/*****************************************************************************

        StageProcSseV4.h
        Author: Laurent de Soras, 2005

Template parameters:
	- CUR: index of the coefficient to process, >= 0

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (hiir_StageProcSseV4_HEADER_INCLUDED)
#define hiir_StageProcSseV4_HEADER_INCLUDED

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

template <int CUR>
class StageProcSseV4
{

	static_assert ((CUR >= 0), "CUR must be >= 0");

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	static hiir_FORCEINLINE void
	               process_sample_pos (StageDataSse *stage_ptr, __m128 &y, __m128 &mem) noexcept;
	static hiir_FORCEINLINE void
	               process_sample_neg (StageDataSse *stage_ptr, __m128 &y, __m128 &mem) noexcept;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	static constexpr int PREV = CUR - 1;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               StageProcSseV4 ()                                 = delete;
	               StageProcSseV4 (const StageProcSseV4 <CUR> &other) = delete;
	               StageProcSseV4 (StageProcSseV4 <CUR> &&other)      = delete;
	               ~StageProcSseV4 ()                                = delete;
	StageProcSseV4 <CUR> &
	               operator = (const StageProcSseV4 <CUR> &other)    = delete;
	StageProcSseV4 <CUR> &
	               operator = (StageProcSseV4 <CUR> &&other)         = delete;
	bool           operator == (const StageProcSseV4 <CUR> &other)   = delete;
	bool           operator != (const StageProcSseV4 <CUR> &other)   = delete;

}; // class StageProcSseV4



}  // namespace hiir



#include "hiir/StageProcSseV4.hpp"



#endif   // hiir_StageProcSseV4_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
