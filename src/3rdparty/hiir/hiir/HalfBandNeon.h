/*****************************************************************************

        HalfBandNeon.h
        Author: Laurent de Soras, 2016

Half-band filter, low-pass or high-pass, using NEON instruction set.

This object must be aligned on a 16-byte boundary!

The output is delayed from 2 sample, compared to the theoretical formula (or
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



#pragma once
#if ! defined (hiir_HalfBandNeon_HEADER_INCLUDED)
#define hiir_HalfBandNeon_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/def.h"
#include "hiir/StageDataNeonV4.h"

#include <arm_neon.h>

#include <array>



namespace hiir
{



template <int NC>
class HalfBandNeon
{

	static_assert ((NC > 0), "Number of coefficient must be positive.");

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef float DataType;
	static constexpr int _nbr_chn  = 1;
	static constexpr int NBR_COEFS = NC;
	static constexpr double _delay = 2;

	               HalfBandNeon () noexcept;
	               HalfBandNeon (const HalfBandNeon <NC> &other) = default;
	               HalfBandNeon (HalfBandNeon <NC> &&other)      = default;
	               ~HalfBandNeon ()                                 = default;

	HalfBandNeon <NC> &
	               operator = (const HalfBandNeon <NC> &other)      = default;
	HalfBandNeon <NC> &
	               operator = (HalfBandNeon <NC> &&other)           = default;

	void           set_coefs (const double coef_arr []) noexcept;

	hiir_FORCEINLINE float
	               process_sample (float input) noexcept;
	void           process_block (float out_ptr [], const float in_ptr [], long nbr_spl) noexcept;

	hiir_FORCEINLINE float
	               process_sample_hpf (float input) noexcept;
	void           process_block_hpf (float out_ptr [], const float in_ptr [], long nbr_spl) noexcept;

	hiir_FORCEINLINE void
	               process_sample_split (float &low, float &hi, float input) noexcept;
	void           process_block_split (float out_l_ptr [], float out_h_ptr [], const float in_ptr [], long nbr_spl) noexcept;

	void           clear_buffers () noexcept;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	static constexpr int _stage_width = 4;
	static constexpr int _nbr_stages  = (NBR_COEFS + _stage_width-1) / _stage_width;
	static constexpr int _nbr_phases  = 2;
	static constexpr int _coef_shift  = ((NBR_COEFS & 1) * 2) ^ 3;

	typedef std::array <StageDataNeonV4, _nbr_stages + 1> Filter;  // Stage 0 contains only input memory
	typedef std::array <Filter, _nbr_phases> FilterBiPhase;

	inline void    set_single_coef (int index, double coef) noexcept;

	hiir_FORCEINLINE float32x2_t
	               process_2_paths (float input) noexcept;
	template <typename FL, typename FH>
	hiir_FORCEINLINE long
	               process_block_quad (float out_l_ptr [], float out_h_ptr [], const float in_ptr [], long nbr_spl, FL fnc_l, FH fnc_h) noexcept;

	hiir_FORCEINLINE static void
	               store_low (float *ptr, float32x4_t even, float32x4_t odd, float32x4_t half) noexcept;
	hiir_FORCEINLINE static void
	               store_high (float *ptr, float32x4_t even, float32x4_t odd, float32x4_t half) noexcept;
	hiir_FORCEINLINE static void
	               bypass (float *, float32x4_t, float32x4_t, float32x4_t) noexcept {}

	FilterBiPhase  _filter;    // Should be the first member (thus easier to align)
	float          _prev;
	int            _phase;     // 0 or 1



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	bool           operator == (const HalfBandNeon <NC> &other) const = delete;
	bool           operator != (const HalfBandNeon <NC> &other) const = delete;

}; // class HalfBandNeon



}  // namespace hiir



#include "hiir/HalfBandNeon.hpp"



#endif   // hiir_HalfBandNeon_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
