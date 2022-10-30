/*****************************************************************************

        StageDataNeonV2.h
        Author: Laurent de Soras, 2020

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (hiir_StageDataNeonV2_HEADER_INCLUDED)
#define hiir_StageDataNeonV2_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include <arm_neon.h>



namespace hiir
{



class StageDataNeonV2
{
public:

	alignas (16) float
	               _coef [2];
	alignas (16) float
	               _mem [2];   // y of the stage

}; // class StageDataNeonV2



}  // namespace hiir



//#include "hiir/StageDataNeonV2.hpp"



#endif   // hiir_StageDataNeonV2_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
