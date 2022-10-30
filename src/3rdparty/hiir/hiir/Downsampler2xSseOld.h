/*****************************************************************************

        Downsampler2xSseOld.h
        Author: Laurent de Soras, 2005

Downsamples by a factor 2 the input signal, using SSE instruction set.

This object must be aligned on a 16-byte boundary!

The output is delayed from 1 sample, compared to the theoretical formula (or
FPU implementation).

Template parameters:
	- NC: number of coefficients, > 0

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (hiir_Downsampler2xSseOld_HEADER_INCLUDED)
#define hiir_Downsampler2xSseOld_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250) // "Inherits via dominance."
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/def.h"
#include "hiir/StageDataSse.h"

#include <array>



namespace hiir
{



template <int NC>
class Downsampler2xSseOld
{

	static_assert ((NC > 0), "Number of coefficient must be positive.");

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef float DataType;
	static constexpr int _nbr_chn  = 1;
	static constexpr int NBR_COEFS = NC;
	static constexpr double _delay = 2 - 1;

	               Downsampler2xSseOld () noexcept;
	               Downsampler2xSseOld (const Downsampler2xSseOld <NC> &other) = default;
	               Downsampler2xSseOld (Downsampler2xSseOld <NC> &&other) = default;
	               ~Downsampler2xSseOld ()                            = default;

	Downsampler2xSseOld <NC> &
	               operator = (const Downsampler2xSseOld <NC> &other) = default;
	Downsampler2xSseOld <NC> &
	               operator = (Downsampler2xSseOld <NC> &&other)      = default;

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

	static constexpr int _stage_width = 4;
	static constexpr int _nbr_stages  =
		(NBR_COEFS + _stage_width - 1) / _stage_width;
	static constexpr int _coef_shift  = ((NBR_COEFS & 1) * 2) ^ 3;

	// Stage 0 contains only input memory
	typedef	std::array <StageDataSse, _nbr_stages + 1>	Filter;

	inline void    set_single_coef (int index, double coef) noexcept;

	template <typename FL, typename FH>
	hiir_FORCEINLINE long
	               process_block_quad (float out_l_ptr [], float out_h_ptr [], const float in_ptr [], long nbr_spl, FL fnc_l, FH fnc_h) noexcept;

	hiir_FORCEINLINE static void
	               store_low (float *ptr, __m128 even, __m128 odd, __m128 half) noexcept;
	hiir_FORCEINLINE static void
	               store_high (float *ptr, __m128 even, __m128 odd, __m128 half) noexcept;
	hiir_FORCEINLINE static void
	               bypass (float *, __m128, __m128, __m128) noexcept {}

	// Should be the first member (thus easier to align)
	alignas (16) Filter
	               _filter;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	bool           operator == (const Downsampler2xSseOld <NC> &other) = delete;
	bool           operator != (const Downsampler2xSseOld <NC> &other) = delete;

}; // class Downsampler2xSseOld



}  // namespace hiir



#include "hiir/Downsampler2xSseOld.hpp"



#endif   // hiir_Downsampler2xSseOld_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
