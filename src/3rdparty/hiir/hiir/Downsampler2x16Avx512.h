/*****************************************************************************

        Downsampler2x16Avx512.h
        Author: Laurent de Soras, 2020

Downsamples vectors of 16 float by a factor 2 the input signal, using AVX
instruction set.

This object must be aligned on a 64-byte boundary!

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
#if ! defined (hiir_Downsampler2x16Avx512_HEADER_INCLUDED)
#define hiir_Downsampler2x16Avx512_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/def.h"
#include "hiir/StageDataAvx512.h"

#include <immintrin.h>

#include <array>



namespace hiir
{



template <int NC>
class Downsampler2x16Avx512
{

	static_assert ((NC > 0), "Number of coefficient must be positive.");

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef float DataType;
	static constexpr int _nbr_chn  = 16;
	static constexpr int NBR_COEFS = NC;
	static constexpr double _delay = -1;

	               Downsampler2x16Avx512 () noexcept;
	               Downsampler2x16Avx512 (const Downsampler2x16Avx512 <NC> &other) = default;
	               Downsampler2x16Avx512 (Downsampler2x16Avx512 <NC> &&other) = default;
	               ~Downsampler2x16Avx512 ()                            = default;

	Downsampler2x16Avx512 <NC> &
	               operator = (const Downsampler2x16Avx512 <NC> &other) = default;
	Downsampler2x16Avx512 <NC> &
	               operator = (Downsampler2x16Avx512 <NC> &&other)      = default;

	void           set_coefs (const double coef_arr []) noexcept;

	hiir_FORCEINLINE __m512
	               process_sample (const float in_ptr [_nbr_chn * 2]) noexcept;
	hiir_FORCEINLINE __m512
	               process_sample (__m512 in_0, __m512 in_1) noexcept;
	void           process_block (float out_ptr [], const float in_ptr [], long nbr_spl) noexcept;

	hiir_FORCEINLINE void
	               process_sample_split (__m512 &low, __m512 &high, const float in_ptr [_nbr_chn * 2]) noexcept;
	hiir_FORCEINLINE void
	               process_sample_split (__m512 &low, __m512 &high, __m512 in_0, __m512 in_1) noexcept;
	void           process_block_split (float out_l_ptr [], float out_h_ptr [], const float in_ptr [], long nbr_spl) noexcept;

	void           clear_buffers () noexcept;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	// Stages 0 and 1 contain only input memories
	typedef std::array <StageDataAvx512, NBR_COEFS + 2> Filter;

	Filter         _filter; // Should be the first member (thus easier to align)



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	bool           operator == (const Downsampler2x16Avx512 <NC> &other) const = delete;
	bool           operator != (const Downsampler2x16Avx512 <NC> &other) const = delete;

}; // class Downsampler2x16Avx512



}  // namespace hiir



#include "hiir/Downsampler2x16Avx512.hpp"



#endif   // hiir_Downsampler2x16Avx512_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
