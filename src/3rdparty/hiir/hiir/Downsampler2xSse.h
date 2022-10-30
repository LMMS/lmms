/*****************************************************************************

        Downsampler2xSse.h
        Author: Laurent de Soras, 2020

Downsamples by a factor 2 the input signal, using SSE instruction set.

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
#if ! defined (hiir_Downsampler2xSse_HEADER_INCLUDED)
#define hiir_Downsampler2xSse_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/def.h"
#include "hiir/StageDataSse.h"

#include <xmmintrin.h>

#include <array>



namespace hiir
{



template <int NC>
class Downsampler2xSse
{

	static_assert ((NC > 0), "Number of coefficient must be positive.");

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef float DataType;
	static constexpr int _nbr_chn  = 1;
	static constexpr int NBR_COEFS = NC;
	static constexpr double _delay = -1;

	               Downsampler2xSse () noexcept;
	               Downsampler2xSse (const Downsampler2xSse <NC> &other) = default;
	               Downsampler2xSse (Downsampler2xSse <NC> &&other)      = default;

	Downsampler2xSse <NC> &
	               operator = (const Downsampler2xSse <NC> &other) = default;
	Downsampler2xSse <NC> &
	               operator = (Downsampler2xSse <NC> &&other)      = default;

	void           set_coefs (const double coef_arr []) noexcept;

	hiir_FORCEINLINE float
	               process_sample (const float in_ptr [2]) noexcept;
	void           process_block (float out_ptr [], const float in_ptr [], long nbr_spl) noexcept;

	hiir_FORCEINLINE void
	               process_sample_split (float &low, float &high, const float in_ptr [2]) noexcept;
	void           process_block_split (float out_l_ptr [], float out_h_ptr [], const float in_ptr [], long nbr_spl) noexcept;

	void           clear_buffers () noexcept;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	static constexpr int _stage_width = 2;
	static constexpr int _nbr_stages  = (NBR_COEFS + _stage_width - 1) / _stage_width;

	template <typename FL, typename FH>
	hiir_FORCEINLINE long
	               process_block_quad (float out_l_ptr [], float out_h_ptr [], const float in_ptr [], long nbr_spl, FL fnc_l, FH fnc_h) noexcept;

	hiir_FORCEINLINE static void
	               store_low (float *ptr, __m128 even, __m128 odd, __m128 half) noexcept;
	hiir_FORCEINLINE static void
	               store_high (float *ptr, __m128 even, __m128 odd, __m128 half) noexcept;
	hiir_FORCEINLINE static void
	               bypass (float *, __m128, __m128, __m128) noexcept {}

	// Stage 0 contains only input memory
	typedef std::array <StageDataSse, _nbr_stages + 1> Filter;

	Filter         _filter;		// Should be the first member (thus easier to align)



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	bool           operator == (const Downsampler2xSse <NC> &other) const = delete;
	bool           operator != (const Downsampler2xSse <NC> &other) const = delete;

}; // class Downsampler2xSse



}  // namespace hiir



#include "hiir/Downsampler2xSse.hpp"



#endif   // hiir_Downsampler2xSse_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
