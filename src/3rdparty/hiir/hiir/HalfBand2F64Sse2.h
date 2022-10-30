/*****************************************************************************

        HalfBand2F64Sse2.h
        Author: Laurent de Soras, 2021

Half-band filter, low-pass or high-pass, using SSE2 instruction set.
Works on vectors of 2 double.

This object must be aligned on a 16-byte boundary!

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
#if ! defined (hiir_HalfBand2F64Sse2_HEADER_INCLUDED)
#define hiir_HalfBand2F64Sse2_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/def.h"
#include "hiir/StageDataF64Sse2.h"

#include <emmintrin.h>

#include <array>



namespace hiir
{



template <int NC>
class HalfBand2F64Sse2
{

	static_assert ((NC > 0), "Number of coefficient must be positive.");

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef double DataType;
	static constexpr int _nbr_chn  = 2;
	static constexpr int NBR_COEFS = NC;
	static constexpr double _delay = 0;

	               HalfBand2F64Sse2 () noexcept;
	               HalfBand2F64Sse2 (const HalfBand2F64Sse2 <NC> &other) = default;
	               HalfBand2F64Sse2 (HalfBand2F64Sse2 <NC> &&other)      = default;
	               ~HalfBand2F64Sse2 ()                              = default;

	HalfBand2F64Sse2 <NC> &
	               operator = (const HalfBand2F64Sse2 <NC> &other)   = default;
	HalfBand2F64Sse2 <NC> &
	               operator = (HalfBand2F64Sse2 <NC> &&other)        = default;

	void           set_coefs (const double coef_arr []) noexcept;

	hiir_FORCEINLINE __m128d
	               process_sample (__m128d input) noexcept;
	void           process_block (double out_ptr [], const double in_ptr [], long nbr_spl) noexcept;

	hiir_FORCEINLINE __m128d
	               process_sample_hpf (__m128d input) noexcept;
	void           process_block_hpf (double out_ptr [], const double in_ptr [], long nbr_spl) noexcept;

	hiir_FORCEINLINE void
	               process_sample_split (__m128d &low, __m128d &high, __m128d input) noexcept;
	void           process_block_split (double out_l_ptr [], double out_h_ptr [], const double in_ptr [], long nbr_spl) noexcept;

	void           clear_buffers () noexcept;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	static constexpr int _nbr_phases = 2;

	typedef std::array <StageDataF64Sse2, NBR_COEFS + 2> Filter;   // Stages 0 and 1 contain only input memories

	typedef	std::array <Filter, _nbr_phases>	FilterBiPhase;

	struct EvenOdd
	{
		alignas (16) __m128d _e;
		alignas (16) __m128d _o;
	};

	hiir_FORCEINLINE EvenOdd
	               process_2_paths (__m128d input) noexcept;
	template <typename FL, typename FH>
	hiir_FORCEINLINE void
	               process_block_2_paths (double out_l_ptr [], double out_h_ptr [], const double in_ptr [], long nbr_spl, FL fnc_l, FH fnc_h) noexcept;

	hiir_FORCEINLINE static void
	               store_low (double *ptr, __m128d tmp_0, __m128d tmp_1) noexcept;
	hiir_FORCEINLINE static void
	               store_high (double *ptr, __m128d tmp_0, __m128d tmp_1) noexcept;
	hiir_FORCEINLINE static void
	               bypass (double *, __m128d, __m128d) noexcept {}

	FilterBiPhase  _bifilter;
	alignas (16) double
	               _prev [_nbr_chn];
	int            _phase;			// 0 or 1



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	bool           operator == (const HalfBand2F64Sse2 &other) const = delete;
	bool           operator != (const HalfBand2F64Sse2 &other) const = delete;

}; // class HalfBand2F64Sse2



}  // namespace hiir



#include "hiir/HalfBand2F64Sse2.hpp"



#endif   // hiir_HalfBand2F64Sse2_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
