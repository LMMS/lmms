/*****************************************************************************

        StageDataAvx512.h
        Port of StageDataAvx512.h from SSE to AVX by Dario Mambro
        StageDataAvx512.h by Laurent de Soras

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if !defined(hiir_StageDataAvx512_HEADER_INCLUDED)
#define hiir_StageDataAvx512_HEADER_INCLUDED

#if defined(_MSC_VER)
#pragma once
#pragma warning(4 : 4250) // "Inherits via dominance."
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include <immintrin.h>



namespace hiir
{



class StageDataAvx512
{

public:

	alignas (64) float
	               _coef [16];
	alignas (64) float
	               _mem [16];  // y of the stage

}; // class StageDataAvx512

} // namespace hiir



#endif // hiir_StageDataAvx512_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
