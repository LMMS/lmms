/*****************************************************************************

        HalfBand4Neon.h
        Author: Laurent de Soras, 2021

Half-band filter, low-pass or high-pass, using NEON instruction set.
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
#if ! defined (hiir_HalfBand4Neon_HEADER_INCLUDED)
#define hiir_HalfBand4Neon_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/def.h"
#include "hiir/StageDataNeonV4.h"

#include <arm_neon.h>

#include <array>



namespace hiir
{



template <int NC>
class HalfBand4Neon
{

	static_assert ((NC > 0), "Number of coefficient must be positive.");

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef float DataType;
	static constexpr int _nbr_chn  = 4;
	static constexpr int NBR_COEFS = NC;
	static constexpr double _delay = 0;

	               HalfBand4Neon () noexcept;
	               HalfBand4Neon (const HalfBand4Neon <NC> &other) = default;
	               HalfBand4Neon (HalfBand4Neon <NC> &&other)      = default;
	               ~HalfBand4Neon ()                              = default;

	HalfBand4Neon <NC> &
	               operator = (const HalfBand4Neon <NC> &other)   = default;
	HalfBand4Neon <NC> &
	               operator = (HalfBand4Neon <NC> &&other)        = default;

	void           set_coefs (const double coef_arr []) noexcept;

	hiir_FORCEINLINE float32x4_t
	               process_sample (float32x4_t input) noexcept;
	void           process_block (float out_ptr [], const float in_ptr [], long nbr_spl) noexcept;

	hiir_FORCEINLINE float32x4_t
	               process_sample_hpf (float32x4_t input) noexcept;
	void           process_block_hpf (float out_ptr [], const float in_ptr [], long nbr_spl) noexcept;

	hiir_FORCEINLINE void
	               process_sample_split (float32x4_t &low, float32x4_t &high, float32x4_t input) noexcept;
	void           process_block_split (float out_l_ptr [], float out_h_ptr [], const float in_ptr [], long nbr_spl) noexcept;

	void           clear_buffers () noexcept;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	static constexpr int _nbr_phases = 2;

	typedef std::array <StageDataNeonV4, NBR_COEFS + 2> Filter;   // Stages 0 and 1 contain only input memories

	typedef	std::array <Filter, _nbr_phases>	FilterBiPhase;

	struct EvenOdd
	{
		alignas (16) float32x4_t _e;
		alignas (16) float32x4_t _o;
	};

	hiir_FORCEINLINE EvenOdd
	               process_2_paths (float32x4_t input) noexcept;
	template <typename FL, typename FH>
	hiir_FORCEINLINE void
	               process_block_2_paths (float out_l_ptr [], float out_h_ptr [], const float in_ptr [], long nbr_spl, FL fnc_l, FH fnc_h) noexcept;

	hiir_FORCEINLINE static void
	               store_low (float *ptr, float32x4_t tmp_0, float32x4_t tmp_1) noexcept;
	hiir_FORCEINLINE static void
	               store_high (float *ptr, float32x4_t tmp_0, float32x4_t tmp_1) noexcept;
	hiir_FORCEINLINE static void
	               bypass (float *, float32x4_t, float32x4_t) noexcept {}

	FilterBiPhase  _bifilter;
	alignas (16) float
	               _prev [_nbr_chn];
	int            _phase;			// 0 or 1



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	bool           operator == (const HalfBand4Neon &other) const = delete;
	bool           operator != (const HalfBand4Neon &other) const = delete;

}; // class HalfBand4Neon



}  // namespace hiir



#include "hiir/HalfBand4Neon.hpp"



#endif   // hiir_HalfBand4Neon_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
