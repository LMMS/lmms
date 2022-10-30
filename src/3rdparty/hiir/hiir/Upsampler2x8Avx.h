/*****************************************************************************

        Upsampler2x8Avx.h
        Ported  Upsampler2x4Sse.h from SSE to AVX by Dario Mambro
        Upsampler2x4Sse.h by Laurent de Soras

Upsamples vectors of 8 float by a factor 2 the input signal, using the AVX
instruction set.

This object must be aligned on a 32-byte boundary!

Template parameters:
	- NC: number of coefficients, > 0

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (hiir_Upsampler2x8Avx_HEADER_INCLUDED)
#define hiir_Upsampler2x8Avx_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/def.h"
#include "hiir/StageDataAvx.h"

#include <immintrin.h>

#include <array>



namespace hiir
{



template <int NC>
class Upsampler2x8Avx
{

	static_assert ((NC > 0), "Number of coefficient must be positive.");

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef float DataType;
	static constexpr int _nbr_chn  = 8;
	static constexpr int NBR_COEFS = NC;
	static constexpr double _delay = 0;

	               Upsampler2x8Avx () noexcept;
	               Upsampler2x8Avx (const Upsampler2x8Avx <NC> &other) = default;
	               Upsampler2x8Avx (Upsampler2x8Avx <NC> &&other)      = default;
	               ~Upsampler2x8Avx ()                            = default;

	Upsampler2x8Avx <NC> &
	               operator = (const Upsampler2x8Avx <NC> &other) = default;
	Upsampler2x8Avx <NC> &
	               operator = (Upsampler2x8Avx <NC> &&other)      = default;

	void				set_coefs (const double coef_arr [NBR_COEFS]) noexcept;
	hiir_FORCEINLINE void
	               process_sample (__m256 &out_0, __m256 &out_1, __m256 input) noexcept;
	void				process_block (float out_ptr [], const float in_ptr [], long nbr_spl) noexcept;
	void				clear_buffers () noexcept;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	typedef std::array <StageDataAvx, NBR_COEFS + 2> Filter;   // Stages 0 and 1 contain only input memories

	Filter         _filter; // Should be the first member (thus easier to align)



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	bool           operator == (const Upsampler2x8Avx <NC> &other) const = delete;
	bool           operator != (const Upsampler2x8Avx <NC> &other) const = delete;

}; // class Upsampler2x8Avx



}  // namespace hiir



#include "hiir/Upsampler2x8Avx.hpp"



#endif   // hiir_Upsampler2x8Avx_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
