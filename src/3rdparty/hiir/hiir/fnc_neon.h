/*****************************************************************************

        fnc_neon.h
        Author: Laurent de Soras, 2020

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (hiir_fnc_neon_HEADER_INCLUDED)
#define hiir_fnc_neon_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/def.h"

#include <arm_neon.h>



namespace hiir
{



hiir_FORCEINLINE float32x4_t  load4a (const float *ptr) noexcept;
hiir_FORCEINLINE float32x4_t  load4u (const float *ptr) noexcept;
hiir_FORCEINLINE float32x2_t  load2a (const float *ptr) noexcept;
hiir_FORCEINLINE float32x2_t  load2u (const float *ptr) noexcept;
hiir_FORCEINLINE void         storea (float *ptr, float32x4_t x) noexcept;
hiir_FORCEINLINE void         storeu (float *ptr, float32x4_t x) noexcept;
hiir_FORCEINLINE void         storea (float *ptr, float32x2_t x) noexcept;
hiir_FORCEINLINE void         storeu (float *ptr, float32x2_t x) noexcept;



}  // namespace hiir



#include "hiir/fnc_neon.hpp"



#endif   // hiir_fnc_neon_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

