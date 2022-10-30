/*****************************************************************************

        StageData3dnow.h
        Author: Laurent de Soras, 2005

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (hiir_StageData3dnow_HEADER_INCLUDED)
#define hiir_StageData3dnow_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250) // "Inherits via dominance."
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include <mm3dnow.h>



namespace hiir
{



class StageData3dnow
{

public:
	__m64          _coefs;  // Coefficients are inverted, by pair: a_{2n+1}, a_{2n}
	__m64          _mem;    // Output of the stage (y)

}; // class StageData3dnow



}  // namespace hiir



#endif   // hiir_StageData3dnow_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
