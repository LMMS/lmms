/*****************************************************************************

        HalfBand4Sse.h
        Author: Laurent de Soras, 2021

Half-band filter, low-pass or high-pass, using SSE instruction set.
Works on vectors of 4 float.

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
#if ! defined (hiir_HalfBand4Sse_HEADER_INCLUDED)
#define hiir_HalfBand4Sse_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/def.h"
#include "hiir/StageDataSse.h"

#include <xmmintrin.h>

#include <array>



namespace hiir
{



template <int NC>
class HalfBand4Sse
{

	static_assert ((NC > 0), "Number of coefficient must be positive.");

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef float DataType;
	static constexpr int _nbr_chn  = 4;
	static constexpr int NBR_COEFS = NC;
	static constexpr double _delay = 0;

	               HalfBand4Sse () noexcept;
	               HalfBand4Sse (const HalfBand4Sse <NC> &other) = default;
	               HalfBand4Sse (HalfBand4Sse <NC> &&other)      = default;
	               ~HalfBand4Sse ()                              = default;

	HalfBand4Sse <NC> &
	               operator = (const HalfBand4Sse <NC> &other)   = default;
	HalfBand4Sse <NC> &
	               operator = (HalfBand4Sse <NC> &&other)        = default;

	void           set_coefs (const double coef_arr []) noexcept;

	hiir_FORCEINLINE __m128
	               process_sample (__m128 input) noexcept;
	void           process_block (float out_ptr [], const float in_ptr [], long nbr_spl) noexcept;

	hiir_FORCEINLINE __m128
	               process_sample_hpf (__m128 input) noexcept;
	void           process_block_hpf (float out_ptr [], const float in_ptr [], long nbr_spl) noexcept;

	hiir_FORCEINLINE void
	               process_sample_split (__m128 &low, __m128 &high, __m128 input) noexcept;
	void           process_block_split (float out_l_ptr [], float out_h_ptr [], const float in_ptr [], long nbr_spl) noexcept;

	void           clear_buffers () noexcept;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	static constexpr int _nbr_phases = 2;

	typedef std::array <StageDataSse, NBR_COEFS + 2> Filter;   // Stages 0 and 1 contain only input memories

	typedef	std::array <Filter, _nbr_phases>	FilterBiPhase;

	struct EvenOdd
	{
		alignas (16) __m128 _e;
		alignas (16) __m128 _o;
	};

	hiir_FORCEINLINE EvenOdd
	               process_2_paths (__m128 input) noexcept;
	template <typename FL, typename FH>
	hiir_FORCEINLINE void
	               process_block_2_paths (float out_l_ptr [], float out_h_ptr [], const float in_ptr [], long nbr_spl, FL fnc_l, FH fnc_h) noexcept;

	hiir_FORCEINLINE static void
	               store_low (float *ptr, __m128 tmp_0, __m128 tmp_1) noexcept;
	hiir_FORCEINLINE static void
	               store_high (float *ptr, __m128 tmp_0, __m128 tmp_1) noexcept;
	hiir_FORCEINLINE static void
	               bypass (float *, __m128, __m128) noexcept {}

	FilterBiPhase  _bifilter;
	alignas (16) float
	               _prev [_nbr_chn];
	int            _phase;			// 0 or 1



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	bool           operator == (const HalfBand4Sse &other) const = delete;
	bool           operator != (const HalfBand4Sse &other) const = delete;

}; // class HalfBand4Sse



}  // namespace hiir



#include "hiir/HalfBand4Sse.hpp"



#endif   // hiir_HalfBand4Sse_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
