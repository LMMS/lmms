/*****************************************************************************

        StageDataNeonV4.h
        Author: Laurent de Soras, 2016

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (hiir_StageDataNeonV4_HEADER_INCLUDED)
#define hiir_StageDataNeonV4_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include <arm_neon.h>



namespace hiir
{



class StageDataNeonV4
{
public:

	alignas (16) float
	               _coef [4];  // a_{4n+1}, a_{4n}, a_{4n+3}, a_{4n+2}
	alignas (16) float
	               _mem [4];   // y of the stage

}; // class StageDataNeonV4



}  // namespace hiir



//#include "hiir/StageDataNeonV4.hpp"



#endif   // hiir_StageDataNeonV4_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
