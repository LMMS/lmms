/*****************************************************************************

        HalfBand16Avx512.h
        Author: Laurent de Soras, 2021

Half-band filter, low-pass or high-pass, using AVX512 instruction set.
Works on vectors of 16 float.

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
#if ! defined (hiir_HalfBand16Avx512_HEADER_INCLUDED)
#define hiir_HalfBand16Avx512_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/def.h"
#include "hiir/StageDataAvx512.h"

#include <immintrin.h>

#include <array>



namespace hiir
{



template <int NC>
class HalfBand16Avx512
{

	static_assert ((NC > 0), "Number of coefficient must be positive.");

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef float DataType;
	static constexpr int _nbr_chn  = 16;
	static constexpr int NBR_COEFS = NC;
	static constexpr double _delay = 0;

	               HalfBand16Avx512 () noexcept;
	               HalfBand16Avx512 (const HalfBand16Avx512 <NC> &other) = default;
	               HalfBand16Avx512 (HalfBand16Avx512 <NC> &&other)      = default;
	               ~HalfBand16Avx512 ()                              = default;

	HalfBand16Avx512 <NC> &
	               operator = (const HalfBand16Avx512 <NC> &other)   = default;
	HalfBand16Avx512 <NC> &
	               operator = (HalfBand16Avx512 <NC> &&other)        = default;

	void           set_coefs (const double coef_arr []) noexcept;

	hiir_FORCEINLINE __m512
	               process_sample (__m512 input) noexcept;
	void           process_block (float out_ptr [], const float in_ptr [], long nbr_spl) noexcept;

	hiir_FORCEINLINE __m512
	               process_sample_hpf (__m512 input) noexcept;
	void           process_block_hpf (float out_ptr [], const float in_ptr [], long nbr_spl) noexcept;

	hiir_FORCEINLINE void
	               process_sample_split (__m512 &low, __m512 &high, __m512 input) noexcept;
	void           process_block_split (float out_l_ptr [], float out_h_ptr [], const float in_ptr [], long nbr_spl) noexcept;

	void           clear_buffers () noexcept;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	static constexpr int _nbr_phases = 2;

	typedef std::array <StageDataAvx512, NBR_COEFS + 2> Filter;   // Stages 0 and 1 contain only input memories

	typedef	std::array <Filter, _nbr_phases>	FilterBiPhase;

	struct EvenOdd
	{
		alignas (64) __m512 _e;
		alignas (64) __m512 _o;
	};

	hiir_FORCEINLINE EvenOdd
	               process_2_paths (__m512 input) noexcept;
	template <typename FL, typename FH>
	hiir_FORCEINLINE void
	               process_block_2_paths (float out_l_ptr [], float out_h_ptr [], const float in_ptr [], long nbr_spl, FL fnc_l, FH fnc_h) noexcept;

	hiir_FORCEINLINE static void
	               store_low (float *ptr, __m512 tmp_0, __m512 tmp_1) noexcept;
	hiir_FORCEINLINE static void
	               store_high (float *ptr, __m512 tmp_0, __m512 tmp_1) noexcept;
	hiir_FORCEINLINE static void
	               bypass (float *, __m512, __m512) noexcept {}

	FilterBiPhase  _bifilter;
	alignas (64) float
	               _prev [_nbr_chn];
	int            _phase;			// 0 or 1



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	bool           operator == (const HalfBand16Avx512 &other) const = delete;
	bool           operator != (const HalfBand16Avx512 &other) const = delete;

}; // class HalfBand16Avx512



}  // namespace hiir



#include "hiir/HalfBand16Avx512.hpp"



#endif   // hiir_HalfBand16Avx512_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
