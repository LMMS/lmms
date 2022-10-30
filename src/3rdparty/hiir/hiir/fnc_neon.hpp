/*****************************************************************************

        fnc_neon.hpp
        Author: Laurent de Soras, 2020

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#if ! defined (hiir_fnc_neon_CODEHEADER_INCLUDED)
#define hiir_fnc_neon_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include <arm_neon.h>

#include <cstdint>



namespace hiir
{



float32x4_t load4a (const float *ptr) noexcept
{
   return vld1q_f32 (ptr);
}



float32x4_t load4u (const float *ptr) noexcept
{
   return vreinterpretq_f32_u8 (vld1q_u8 (
      reinterpret_cast <const uint8_t *> (ptr)
   ));
}



float32x2_t load2a (const float *ptr) noexcept
{
   return vld1_f32 (ptr);
}



float32x2_t load2u (const float *ptr) noexcept
{
   return vreinterpret_f32_u8 (vld1_u8 (
      reinterpret_cast <const uint8_t *> (ptr)
   ));
}



void  storea (float *ptr, float32x4_t x) noexcept
{
   vst1q_f32 (ptr, x);
}



void  storeu (float *ptr, float32x4_t x) noexcept
{
   vst1q_u8 (reinterpret_cast <uint8_t *> (ptr), vreinterpretq_u8_f32 (x));
}



void  storea (float *ptr, float32x2_t x) noexcept
{
   vst1_f32 (ptr, x);
}



void  storeu (float *ptr, float32x2_t x) noexcept
{
   vst1_u8 (reinterpret_cast <uint8_t *> (ptr), vreinterpret_u8_f32 (x));
}



}  // namespace hiir



#endif   // hiir_fnc_neon_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

