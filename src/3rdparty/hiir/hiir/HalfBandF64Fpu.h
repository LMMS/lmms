/*****************************************************************************

        HalfBandF64Fpu.h
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (hiir_HalfBandF64Fpu_HEADER_INCLUDED)
#define hiir_HalfBandF64Fpu_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/HalfBandTpl.h"



namespace hiir
{



template <int NC>
using HalfBandF64Fpu = HalfBandTpl <NC, double, 1>;



}  // namespace hiir



//#include "hiir/HalfBandF64Fpu.hpp"



#endif   // hiir_HalfBandF64Fpu_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
