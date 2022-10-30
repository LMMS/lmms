/*****************************************************************************

        StageProc3dnow.h
        Author: Laurent de Soras, 2005

Template parameters:
	- CUR: Number of remaining coefficients to process, >= 0

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (hiir_StageProc3dnow_HEADER_INCLUDED)
#define hiir_StageProc3dnow_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250) // "Inherits via dominance."
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/def.h"



namespace hiir
{



template <int CUR>
class StageProc3dnow
{

	static_assert ((CUR >= 0), "CUR must be >= 0");

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	static hiir_FORCEINLINE void
	               process_sample_pos () noexcept;
	static hiir_FORCEINLINE void
	               process_sample_neg () noexcept;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               StageProc3dnow ()                                     = delete;
	               StageProc3dnow (const StageProc3dnow <CUR> &other)    = delete;
	               StageProc3dnow (StageProc3dnow <CUR> &&other)         = delete;
	               ~StageProc3dnow ()                                    = delete;
	StageProc3dnow &
	               operator = (const StageProc3dnow <CUR> &other)        = delete;
	StageProc3dnow &
	               operator = (StageProc3dnow <CUR> &&other)             = delete;
	bool           operator == (const StageProc3dnow <CUR> &other) const = delete;
	bool           operator != (const StageProc3dnow <CUR> &other) const = delete;

}; // class StageProc3dnow



}  // namespace hiir



#include "hiir/StageProc3dnow.hpp"



#endif   // hiir_StageProc3dnow_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
