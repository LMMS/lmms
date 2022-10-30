/*****************************************************************************

        Downsampler2xF64Sse2.h
        Author: Laurent de Soras, 2020

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (hiir_Downsampler2xF64Sse2_HEADER_INCLUDED)
#define hiir_Downsampler2xF64Sse2_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/def.h"
#include "hiir/StageDataF64Sse2.h"

#include <emmintrin.h>

#include <array>



namespace hiir
{



template <int NC>
class Downsampler2xF64Sse2
{

	static_assert ((NC > 0), "Number of coefficient must be positive.");

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef double DataType;
	static constexpr int _nbr_chn  = 1;
	static constexpr int NBR_COEFS = NC;
	static constexpr double _delay = -1;

	               Downsampler2xF64Sse2 () noexcept;
	               Downsampler2xF64Sse2 (const Downsampler2xF64Sse2 <NC> &other) = default;
	               Downsampler2xF64Sse2 (Downsampler2xF64Sse2 <NC> &&other)      = default;

	Downsampler2xF64Sse2 <NC> &
	               operator = (const Downsampler2xF64Sse2 <NC> &other) = default;
	Downsampler2xF64Sse2 <NC> &
	               operator = (Downsampler2xF64Sse2 <NC> &&other)      = default;

	void           set_coefs (const double coef_arr []) noexcept;

	hiir_FORCEINLINE double
	               process_sample (const double in_ptr [2]) noexcept;
	void           process_block (double out_ptr [], const double in_ptr [], long nbr_spl) noexcept;

	hiir_FORCEINLINE void
	               process_sample_split (double &low, double &high, const double in_ptr [2]) noexcept;
	void           process_block_split (double out_l_ptr [], double out_h_ptr [], const double in_ptr [], long nbr_spl) noexcept;

	void           clear_buffers () noexcept;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	static constexpr int _stage_width = 2;
	static constexpr int _nbr_stages  =
		(NBR_COEFS + _stage_width - 1) / _stage_width;

	template <typename FL, typename FH>
	hiir_FORCEINLINE long
	               process_block_double (double out_l_ptr [], double out_h_ptr [], const double in_ptr [], long nbr_spl, FL fnc_l, FH fnc_h) noexcept;

	hiir_FORCEINLINE static void
	               store_low (double *ptr, __m128d even, __m128d odd, __m128d half) noexcept;
	hiir_FORCEINLINE static void
	               store_high (double *ptr, __m128d even, __m128d odd, __m128d half) noexcept;
	hiir_FORCEINLINE static void
	               bypass (double *, __m128d, __m128d, __m128d) noexcept {}

	// Stage 0 contains only input memory
	typedef std::array <StageDataF64Sse2, _nbr_stages + 1> Filter;

	Filter         _filter;		// Should be the first member (thus easier to align)



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	bool           operator == (const Downsampler2xF64Sse2 <NC> &other) const = delete;
	bool           operator != (const Downsampler2xF64Sse2 <NC> &other) const = delete;

}; // class Downsampler2xF64Sse2



}  // namespace hiir



#include "hiir/Downsampler2xF64Sse2.hpp"



#endif   // hiir_Downsampler2xF64Sse2_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
