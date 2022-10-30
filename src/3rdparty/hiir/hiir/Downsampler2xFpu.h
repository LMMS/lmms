/*****************************************************************************

        Downsampler2xFpu.h
        Author: Laurent de Soras, 2020

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (hiir_Downsampler2xFpu_HEADER_INCLUDED)
#define hiir_Downsampler2xFpu_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/Downsampler2xTpl.h"



namespace hiir
{



template <int NC>
using Downsampler2xFpu = Downsampler2xTpl <NC, float, 1>;



}  // namespace hiir



//#include "hiir/Downsampler2xFpu.hpp"



#endif   // hiir_Downsampler2xFpu_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
