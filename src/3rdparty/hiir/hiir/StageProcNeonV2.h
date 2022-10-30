/*****************************************************************************

        StageProcNeonV2.h
        Author: Laurent de Soras, 2016

Template parameters:
	- CUR: index of the coefficient coefficient to process, >= 0

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (hiir_StageProcNeonV2_HEADER_INCLUDED)
#define hiir_StageProcNeonV2_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/def.h"



namespace hiir
{



class StageDataNeonV2;

template <int CUR>
class StageProcNeonV2
{

	static_assert ((CUR >= 0), "CUR must be >= 0");

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	static hiir_FORCEINLINE void
	               process_sample_pos (float32x2_t &x, StageDataNeonV2 *stage_ptr) noexcept;
	static hiir_FORCEINLINE void
	               process_sample_neg (float32x2_t &x, StageDataNeonV2 *stage_ptr) noexcept;

	static hiir_FORCEINLINE void
	               process_sample_pos_rec (float32x2_t &x, StageDataNeonV2 *stage_ptr) noexcept;
	static hiir_FORCEINLINE void
	               process_sample_neg_rec (float32x2_t &x, StageDataNeonV2 *stage_ptr) noexcept;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               StageProcNeonV2 ()                                     = delete;
	               StageProcNeonV2 (const StageProcNeonV2 <CUR> &other)   = delete;
	               StageProcNeonV2 (StageProcNeonV2 <CUR> &&other)        = delete;
	               ~StageProcNeonV2 ()                                    = delete;
	StageProcNeonV2 &
	               operator = (const StageProcNeonV2 <CUR> &other)        = delete;
	StageProcNeonV2 &
	               operator = (StageProcNeonV2 <CUR> &&other)             = delete;
	bool           operator == (const StageProcNeonV2 <CUR> &other) const = delete;
	bool           operator != (const StageProcNeonV2 <CUR> &other) const = delete;

}; // class StageProcNeonV2



}  // namespace hiir



#include "hiir/StageProcNeonV2.hpp"



#endif   // hiir_StageProcNeonV2_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
