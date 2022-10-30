/*****************************************************************************

        StageProcNeonV4.h
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
#if ! defined (hiir_StageProcNeonV4_HEADER_INCLUDED)
#define hiir_StageProcNeonV4_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/def.h"



namespace hiir
{



class StageDataNeonV4;

template <int CUR>
class StageProcNeonV4
{

	static_assert ((CUR >= 0), "CUR must be >= 0");

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	static hiir_FORCEINLINE void
	               process_sample_pos (StageDataNeonV4 *stage_ptr, float32x4_t &y, float32x4_t &mem) noexcept;
	static hiir_FORCEINLINE void
	               process_sample_neg (StageDataNeonV4 *stage_ptr, float32x4_t &y, float32x4_t &mem) noexcept;

	static hiir_FORCEINLINE void
	               process_sample_pos_rec (StageDataNeonV4 *stage_ptr, float32x4_t &y, float32x4_t &mem) noexcept;
	static hiir_FORCEINLINE void
	               process_sample_neg_rec (StageDataNeonV4 *stage_ptr, float32x4_t &y, float32x4_t &mem) noexcept;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               StageProcNeonV4 ()                                     = delete;
	               StageProcNeonV4 (const StageProcNeonV4 <CUR> &other)   = delete;
	               StageProcNeonV4 (StageProcNeonV4 <CUR> &&other)        = delete;
	               ~StageProcNeonV4 ()                                    = delete;
	StageProcNeonV4 &
	               operator = (const StageProcNeonV4 <CUR> &other)        = delete;
	StageProcNeonV4 &
	               operator = (StageProcNeonV4 <CUR> &&other)             = delete;
	bool           operator == (const StageProcNeonV4 <CUR> &other) const = delete;
	bool           operator != (const StageProcNeonV4 <CUR> &other) const = delete;

}; // class StageProcNeonV4



}  // namespace hiir



#include "hiir/StageProcNeonV4.hpp"



#endif   // hiir_StageProcNeonV4_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
