/*****************************************************************************

        Downsampler2x8F64Avx512.h
        Author: Laurent de Soras, 2020

Downsamples vectors of 8 double by a factor 2 the input signal, using AVX
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
#if ! defined (hiir_Downsampler2x8F64Avx512_HEADER_INCLUDED)
#define hiir_Downsampler2x8F64Avx512_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/def.h"
#include "hiir/StageDataF64Avx512.h"

#include <immintrin.h> 

#include <array>



namespace hiir
{



template <int NC>
class Downsampler2x8F64Avx512
{

	static_assert ((NC > 0), "Number of coefficient must be positive.");

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef double DataType;
	static constexpr int _nbr_chn  = 8;
	static constexpr int NBR_COEFS = NC;
	static constexpr double _delay = -1;

	               Downsampler2x8F64Avx512 () noexcept;
	               Downsampler2x8F64Avx512 (const Downsampler2x8F64Avx512 <NC> &other) = default;
	               Downsampler2x8F64Avx512 (Downsampler2x8F64Avx512 <NC> &&other) = default;
	               ~Downsampler2x8F64Avx512 ()                            = default;

	Downsampler2x8F64Avx512 <NC> &
	               operator = (const Downsampler2x8F64Avx512 <NC> &other) = default;
	Downsampler2x8F64Avx512 <NC> &
	               operator = (Downsampler2x8F64Avx512 <NC> &&other)      = default;

	void           set_coefs (const double coef_arr []) noexcept;

	hiir_FORCEINLINE __m512d
	               process_sample (const double in_ptr [_nbr_chn * 2]) noexcept;
	hiir_FORCEINLINE __m512d
	               process_sample (__m512d in_0, __m512d in_1) noexcept;
	void           process_block (double out_ptr [], const double in_ptr [], long nbr_spl) noexcept;

	hiir_FORCEINLINE void
	               process_sample_split (__m512d &low, __m512d &high, const double in_ptr [_nbr_chn * 2]) noexcept;
	hiir_FORCEINLINE void
	               process_sample_split (__m512d &low, __m512d &high, __m512d in_0, __m512d in_1) noexcept;
	void           process_block_split (double out_l_ptr [], double out_h_ptr [], const double in_ptr [], long nbr_spl) noexcept;

	void           clear_buffers () noexcept;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	typedef std::array <StageDataF64Avx512, NBR_COEFS + 2> Filter;   // Stages 0 and 1 contain only input memories

	Filter         _filter; // Should be the first member (thus easier to align)



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	bool           operator == (const Downsampler2x8F64Avx512 <NC> &other) const = delete;
	bool           operator != (const Downsampler2x8F64Avx512 <NC> &other) const = delete;

}; // class Downsampler2x8F64Avx512



}  // namespace hiir



#include "hiir/Downsampler2x8F64Avx512.hpp"



#endif   // hiir_Downsampler2x8F64Avx512_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
