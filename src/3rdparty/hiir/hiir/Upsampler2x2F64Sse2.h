/*****************************************************************************

        Upsampler2x2F64Sse2.h
        Port of Upsampler2x4Sse.h from float to double by Dario Mambro
        Upsampler2x4Sse.h by Laurent de Soras, 2015

Upsamples vectors of 2 double by a factor 2 the input signal, using the SSE
instruction set.

This object must be aligned on a 16-byte boundary!

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
#if ! defined (hiir_Upsampler2x2F64Sse2_HEADER_INCLUDED)
#define hiir_Upsampler2x2F64Sse2_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/def.h"
#include "hiir/StageDataF64Sse2.h"

#include <emmintrin.h>

#include <array>



namespace hiir
{



template <int NC>
class Upsampler2x2F64Sse2
{

	static_assert ((NC > 0), "Number of coefficient must be positive.");

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef double DataType;
	static constexpr int _nbr_chn  = 2;
	static constexpr int NBR_COEFS = NC;
	static constexpr double _delay = 0;

	               Upsampler2x2F64Sse2 () noexcept;
	               Upsampler2x2F64Sse2 (const Upsampler2x2F64Sse2 <NC> &other) = default;
	               Upsampler2x2F64Sse2 (Upsampler2x2F64Sse2 <NC> &&other)      = default;
	               ~Upsampler2x2F64Sse2 ()                             = default;

	Upsampler2x2F64Sse2 <NC> &
	               operator = (const Upsampler2x2F64Sse2 <NC> &other) = default;
	Upsampler2x2F64Sse2 <NC> &
	               operator = (Upsampler2x2F64Sse2 <NC> &&other)      = default;


	void				set_coefs (const double coef_arr [NBR_COEFS]) noexcept;
	hiir_FORCEINLINE void
	               process_sample (__m128d &out_0, __m128d &out_1, __m128d input) noexcept;
	void				process_block (double out_ptr [], const double in_ptr [], long nbr_spl) noexcept;
	void				clear_buffers () noexcept;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	typedef std::array <StageDataF64Sse2, NBR_COEFS + 2> Filter;   // Stages 0 and 1 contain only input memories

	Filter         _filter; // Should be the first member (thus easier to align)



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	bool           operator == (const Upsampler2x2F64Sse2 <NC> &other) const = delete;
	bool           operator != (const Upsampler2x2F64Sse2 <NC> &other) const = delete;

}; // class Upsampler2x2F64Sse2



}  // namespace hiir



#include "hiir/Upsampler2x2F64Sse2.hpp"



#endif   // hiir_Upsampler2x2F64Sse2_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
