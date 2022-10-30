/*****************************************************************************

        HalfBand8F64Avx512.h
        Author: Laurent de Soras, 2021

Half-band filter, low-pass or high-pass, using AVX instruction set.
Works on vectors of 8 double.

This object must be aligned on a 64-byte boundary!

Template parameters:
	- NC: number of coefficients, > 0

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (hiir_HalfBand8F64Avx512_HEADER_INCLUDED)
#define hiir_HalfBand8F64Avx512_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/def.h"
#include "hiir/StageDataF64Avx512.h"

#include <immintrin.h>

#include <array>



namespace hiir
{



template <int NC>
class HalfBand8F64Avx512
{

	static_assert ((NC > 0), "Number of coefficient must be positive.");

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef double DataType;
	static constexpr int _nbr_chn  = 8;
	static constexpr int NBR_COEFS = NC;
	static constexpr double _delay = 0;

	               HalfBand8F64Avx512 () noexcept;
	               HalfBand8F64Avx512 (const HalfBand8F64Avx512 <NC> &other) = default;
	               HalfBand8F64Avx512 (HalfBand8F64Avx512 <NC> &&other)      = default;
	               ~HalfBand8F64Avx512 ()                            = default;

	HalfBand8F64Avx512 <NC> &
	               operator = (const HalfBand8F64Avx512 <NC> &other) = default;
	HalfBand8F64Avx512 <NC> &
	               operator = (HalfBand8F64Avx512 <NC> &&other)      = default;

	void           set_coefs (const double coef_arr []) noexcept;

	hiir_FORCEINLINE __m512d
	               process_sample (__m512d input) noexcept;
	void           process_block (double out_ptr [], const double in_ptr [], long nbr_spl) noexcept;

	hiir_FORCEINLINE __m512d
	               process_sample_hpf (__m512d input) noexcept;
	void           process_block_hpf (double out_ptr [], const double in_ptr [], long nbr_spl) noexcept;

	hiir_FORCEINLINE void
	               process_sample_split (__m512d &low, __m512d &high, __m512d input) noexcept;
	void           process_block_split (double out_l_ptr [], double out_h_ptr [], const double in_ptr [], long nbr_spl) noexcept;

	void           clear_buffers () noexcept;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	static constexpr int _nbr_phases = 2;

	typedef std::array <StageDataF64Avx512, NBR_COEFS + 2> Filter;   // Stages 0 and 1 contain only input memories

	typedef	std::array <Filter, _nbr_phases>	FilterBiPhase;

	struct EvenOdd
	{
		alignas (64) __m512d _e;
		alignas (64) __m512d _o;
	};

	hiir_FORCEINLINE EvenOdd
	               process_2_paths (__m512d input) noexcept;
	template <typename FL, typename FH>
	hiir_FORCEINLINE void
	               process_block_2_paths (double out_l_ptr [], double out_h_ptr [], const double in_ptr [], long nbr_spl, FL fnc_l, FH fnc_h) noexcept;

	hiir_FORCEINLINE static void
	               store_low (double *ptr, __m512d tmp_0, __m512d tmp_1) noexcept;
	hiir_FORCEINLINE static void
	               store_high (double *ptr, __m512d tmp_0, __m512d tmp_1) noexcept;
	hiir_FORCEINLINE static void
	               bypass (double *, __m512d, __m512d) noexcept {}

	FilterBiPhase  _bifilter;
	alignas (64) double
	               _prev [_nbr_chn];
	int            _phase;			// 0 or 1



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	bool           operator == (const HalfBand8F64Avx512 &other) const = delete;
	bool           operator != (const HalfBand8F64Avx512 &other) const = delete;

}; // class HalfBand8F64Avx512



}  // namespace hiir



#include "hiir/HalfBand8F64Avx512.hpp"



#endif   // hiir_HalfBand8F64Avx512_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
