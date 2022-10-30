/*****************************************************************************

        PhaseHalfPiFpu.h
        Author: Laurent de Soras, 2020

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (hiir_PhaseHalfPiFpu_HEADER_INCLUDED)
#define hiir_PhaseHalfPiFpu_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/PhaseHalfPiTpl.h"



namespace hiir
{



template <int NC>
using PhaseHalfPiFpu = PhaseHalfPiTpl <NC, float, 1>;



}  // namespace hiir



//#include "hiir/PhaseHalfPiFpu.hpp"



#endif   // hiir_PhaseHalfPiFpu_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
