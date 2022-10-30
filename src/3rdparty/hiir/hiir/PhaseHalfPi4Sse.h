/*****************************************************************************

        PhaseHalfPi4Sse.h
        Author: Laurent de Soras, 2020

From the input signal, generates two signals with a pi/2 phase shift, using
SSE instruction set. Works on vectors of 4 float.

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
#if ! defined (hiir_PhaseHalfPi4Sse_HEADER_INCLUDED)
#define hiir_PhaseHalfPi4Sse_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/def.h"
#include "hiir/StageDataSse.h"

#include <xmmintrin.h>

#include <array>



namespace hiir
{



template <int NC>
class PhaseHalfPi4Sse
{

	static_assert ((NC > 0), "Number of coefficient must be positive.");

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef float DataType;
	static constexpr int _nbr_chn  = 4;
	static constexpr int NBR_COEFS = NC;
	static constexpr double _delay = 0;

	               PhaseHalfPi4Sse () noexcept;
	               PhaseHalfPi4Sse (const PhaseHalfPi4Sse <NC> &other) = default;
	               PhaseHalfPi4Sse (PhaseHalfPi4Sse <NC> &&other)      = default;
	               ~PhaseHalfPi4Sse ()                                 = default;

	PhaseHalfPi4Sse <NC> &
	               operator = (const PhaseHalfPi4Sse <NC> &other)      = default;
	PhaseHalfPi4Sse <NC> &
	               operator = (PhaseHalfPi4Sse <NC> &&other)           = default;

	void           set_coefs (const double coef_arr []) noexcept;

	hiir_FORCEINLINE void
	               process_sample (__m128 &out_0, __m128 &out_1, __m128 input) noexcept;
	void           process_block (float out_0_ptr [], float out_1_ptr [], const float in_ptr [], long nbr_spl) noexcept;

	void           clear_buffers () noexcept;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	static constexpr int _nbr_phases = 2;

	typedef std::array <StageDataSse, NBR_COEFS + 2> Filter;   // Stages 0 and 1 contain only input memories

	typedef	std::array <Filter, _nbr_phases>	FilterBiPhase;

	FilterBiPhase  _bifilter;
	alignas (16) float
	               _prev [_nbr_chn];
	int            _phase;			// 0 or 1



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	bool           operator == (const PhaseHalfPi4Sse &other) const = delete;
	bool           operator != (const PhaseHalfPi4Sse &other) const = delete;

}; // class PhaseHalfPi4Sse



}  // namespace hiir



#include "hiir/PhaseHalfPi4Sse.hpp"



#endif   // hiir_PhaseHalfPi4Sse_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
