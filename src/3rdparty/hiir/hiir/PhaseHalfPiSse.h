/*****************************************************************************

        PhaseHalfPiSse.h
        Author: Laurent de Soras, 2005

From the input signal, generates two signals with a pi/2 phase shift, using
SSE instruction set.

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



#if ! defined (hiir_PhaseHalfPiSse_HEADER_INCLUDED)
#define hiir_PhaseHalfPiSse_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250) // "Inherits via dominance."
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/def.h"
#include "hiir/StageDataSse.h"

#include <xmmintrin.h>

#include <array>



namespace hiir
{



template <int NC>
class PhaseHalfPiSse
{

	static_assert ((NC > 0), "Number of coefficient must be positive.");

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef float DataType;
	static constexpr int _nbr_chn  = 1;
	static constexpr int NBR_COEFS = NC;
	static constexpr double _delay = 2;

	               PhaseHalfPiSse () noexcept;
	               PhaseHalfPiSse (const PhaseHalfPiSse <NC> &other) = default;
	               PhaseHalfPiSse (PhaseHalfPiSse <NC> &&other)      = default;
	               ~PhaseHalfPiSse ()                                = default;

	PhaseHalfPiSse <NC> &
	               operator = (const PhaseHalfPiSse <NC> &other)     = default;
	PhaseHalfPiSse <NC> &
	               operator = (PhaseHalfPiSse <NC> &&other)          = default;

	void           set_coefs (const double coef_arr []) noexcept;

	hiir_FORCEINLINE void
	               process_sample (float &out_0, float &out_1, float input) noexcept;
	void           process_block (float out_0_ptr [], float out_1_ptr [], const float in_ptr [], long nbr_spl) noexcept;

	void           clear_buffers () noexcept;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	static constexpr int _stage_width = 4;
	static constexpr int _nbr_stages  = (NBR_COEFS + _stage_width-1) / _stage_width;
	static constexpr int _nbr_phases  = 2;
	static constexpr int _coef_shift  = ((NBR_COEFS & 1) * 2) ^ 3;

	// Stage 0 contains only input memory
	typedef std::array <StageDataSse, _nbr_stages + 1> Filter;
   typedef std::array <Filter, _nbr_phases> FilterBiPhase;

	inline void    set_single_coef (int index, double coef) noexcept;

	hiir_FORCEINLINE long
	               process_block_quad (float out_0_ptr [], float out_1_ptr [], const float in_ptr [], long nbr_spl) noexcept;

	// Should be the first member (thus easier to align)
	alignas (16) FilterBiPhase
	               _filter;
	float          _prev;
	int            _phase;  // 0 or 1



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	bool           operator == (const PhaseHalfPiSse <NC> &other) = delete;
	bool           operator != (const PhaseHalfPiSse <NC> &other) = delete;

}; // class PhaseHalfPiSse



}  // namespace hiir



#include "hiir/PhaseHalfPiSse.hpp"



#endif   // hiir_PhaseHalfPiSse_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
