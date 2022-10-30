/*****************************************************************************

        HalfBandF64Sse2.h
        Author: Laurent de Soras, 2021

Half-band filter, low-pass or high-pass, using SSE2 instruction set.

This object must be aligned on a 16-byte boundary!

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
#if ! defined (hiir_HalfBandF64Sse2_HEADER_INCLUDED)
#define hiir_HalfBandF64Sse2_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/def.h"
#include "hiir/StageDataF64Sse2.h"

#include <emmintrin.h>

#include <array>



namespace hiir
{



template <int NC>
class HalfBandF64Sse2
{

	static_assert ((NC > 0), "Number of coefficient must be positive.");

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef double DataType;
	static constexpr int _nbr_chn  = 1;
	static constexpr int NBR_COEFS = NC;
	static constexpr double _delay = 0;

	               HalfBandF64Sse2 () noexcept;
	               HalfBandF64Sse2 (const HalfBandF64Sse2 <NC> &other) = default;
	               HalfBandF64Sse2 (HalfBandF64Sse2 <NC> &&other)      = default;
	               ~HalfBandF64Sse2 ()                            = default;

	HalfBandF64Sse2 <NC> &
	               operator = (const HalfBandF64Sse2 <NC> &other) = default;
	HalfBandF64Sse2 <NC> &
	               operator = (HalfBandF64Sse2 <NC> &&other)      = default;

	void           set_coefs (const double coef_arr []) noexcept;

	hiir_FORCEINLINE double
	               process_sample (double input) noexcept;
	void           process_block (double out_ptr [], const double in_ptr [], long nbr_spl) noexcept;

	hiir_FORCEINLINE double
	               process_sample_hpf (double input) noexcept;
	void           process_block_hpf (double out_ptr [], const double in_ptr [], long nbr_spl) noexcept;

	hiir_FORCEINLINE void
	               process_sample_split (double &low, double &high, double input) noexcept;
	void           process_block_split (double out_l_ptr [], double out_h_ptr [], const double in_ptr [], long nbr_spl) noexcept;

	void           clear_buffers () noexcept;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	static constexpr int _stage_width = 2;
	static constexpr int _nbr_stages  = (NBR_COEFS + _stage_width-1) / _stage_width;
	static constexpr int _nbr_phases  = 2;

	// Stage 0 contains only input memory
	typedef std::array <StageDataF64Sse2, _nbr_stages + 1> Filter;
   typedef std::array <Filter, _nbr_phases> FilterBiPhase;

	hiir_FORCEINLINE std::array <double, 2>
	               process_2_paths (double input) noexcept;
	template <typename FL, typename FH>
	hiir_FORCEINLINE void
	               process_block_2_paths (double out_l_ptr [], double out_h_ptr [], const double in_ptr [], long nbr_spl, FL fnc_l, FH fnc_h) noexcept;

	static hiir_FORCEINLINE void
	               store_low (double *ptr, double even, double odd) noexcept;
	static hiir_FORCEINLINE void
	               store_high (double *ptr, double even, double odd) noexcept;
	static hiir_FORCEINLINE void
	               bypass (double *, double, double) noexcept {}

	// Should be the first member (thus easier to align)
	alignas (16) FilterBiPhase
	               _bifilter;
	double         _prev;
	int            _phase;  // 0 or 1



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	bool           operator == (const HalfBandF64Sse2 <NC> &other) = delete;
	bool           operator != (const HalfBandF64Sse2 <NC> &other) = delete;

}; // class HalfBandF64Sse2



}  // namespace hiir



#include "hiir/HalfBandF64Sse2.hpp"



#endif   // hiir_HalfBandF64Sse2_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
