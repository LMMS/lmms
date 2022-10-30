/*****************************************************************************

        StageDataF64Sse2.h
        Port of StageDataSse.h from float to double by Dario Mambro
        StageDataSse.h by Laurent de Soras, 2005

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (hiir_StageDataF64Sse2_HEADER_INCLUDED)
#define hiir_StageDataF64Sse2_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250) // "Inherits via dominance."
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include <emmintrin.h>



namespace hiir
{



class StageDataF64Sse2
{

public:

	alignas (16) double
	               _coef [2];  
	alignas (16) double
	               _mem [2];   // y of the stage

}; // class StageDataF64Sse2



}  // namespace hiir



#endif   // hiir_StageDataF64Sse2_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
