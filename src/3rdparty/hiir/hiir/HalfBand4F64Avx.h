/*****************************************************************************

        HalfBand4F64Avx.h
        Author: Laurent de Soras, 2021

Half-band filter, low-pass or high-pass, using AVX instruction set.
Works on vectors of 4 double.

This object must be aligned on a 32-byte boundary!

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
#if ! defined (hiir_HalfBand4F64Avx_HEADER_INCLUDED)
#define hiir_HalfBand4F64Avx_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/def.h"
#include "hiir/StageDataF64Avx.h"

#include <immintrin.h>

#include <array>



namespace hiir
{



template <int NC>
class HalfBand4F64Avx
{

	static_assert ((NC > 0), "Number of coefficient must be positive.");

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef double DataType;
	static constexpr int _nbr_chn  = 4;
	static constexpr int NBR_COEFS = NC;
	static constexpr double _delay = 0;

	               HalfBand4F64Avx () noexcept;
	               HalfBand4F64Avx (const HalfBand4F64Avx <NC> &other) = default;
	               HalfBand4F64Avx (HalfBand4F64Avx <NC> &&other)      = default;
	               ~HalfBand4F64Avx ()                              = default;

	HalfBand4F64Avx <NC> &
	               operator = (const HalfBand4F64Avx <NC> &other)   = default;
	HalfBand4F64Avx <NC> &
	               operator = (HalfBand4F64Avx <NC> &&other)        = default;

	void           set_coefs (const double coef_arr []) noexcept;

	hiir_FORCEINLINE __m256d
	               process_sample (__m256d input) noexcept;
	void           process_block (double out_ptr [], const double in_ptr [], long nbr_spl) noexcept;

	hiir_FORCEINLINE __m256d
	               process_sample_hpf (__m256d input) noexcept;
	void           process_block_hpf (double out_ptr [], const double in_ptr [], long nbr_spl) noexcept;

	hiir_FORCEINLINE void
	               process_sample_split (__m256d &low, __m256d &high, __m256d input) noexcept;
	void           process_block_split (double out_l_ptr [], double out_h_ptr [], const double in_ptr [], long nbr_spl) noexcept;

	void           clear_buffers () noexcept;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	static constexpr int _nbr_phases = 2;

	typedef std::array <StageDataF64Avx, NBR_COEFS + 2> Filter;   // Stages 0 and 1 contain only input memories

	typedef	std::array <Filter, _nbr_phases>	FilterBiPhase;

	struct EvenOdd
	{
		alignas (32) __m256d _e;
		alignas (32) __m256d _o;
	};

	hiir_FORCEINLINE EvenOdd
	               process_2_paths (__m256d input) noexcept;
	template <typename FL, typename FH>
	hiir_FORCEINLINE void
	               process_block_2_paths (double out_l_ptr [], double out_h_ptr [], const double in_ptr [], long nbr_spl, FL fnc_l, FH fnc_h) noexcept;

	hiir_FORCEINLINE static void
	               store_low (double *ptr, __m256d tmp_0, __m256d tmp_1) noexcept;
	hiir_FORCEINLINE static void
	               store_high (double *ptr, __m256d tmp_0, __m256d tmp_1) noexcept;
	hiir_FORCEINLINE static void
	               bypass (double *, __m256d, __m256d) noexcept {}

	FilterBiPhase  _bifilter;
	alignas (32) double
	               _prev [_nbr_chn];
	int            _phase;			// 0 or 1



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	bool           operator == (const HalfBand4F64Avx &other) const = delete;
	bool           operator != (const HalfBand4F64Avx &other) const = delete;

}; // class HalfBand4F64Avx



}  // namespace hiir



#include "hiir/HalfBand4F64Avx.hpp"



#endif   // hiir_HalfBand4F64Avx_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
