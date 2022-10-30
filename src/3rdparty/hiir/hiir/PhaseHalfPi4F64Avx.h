/*****************************************************************************

        PhaseHalfPi4F64Avx.h
        Author: Laurent de Soras, 2020

From the input signal, generates two signals with a pi/2 phase shift, using
AVX instruction set. Works on vectors of 4 double.

This object must be aligned on a 32-byte boundary!

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
#if ! defined (hiir_PhaseHalfPi4F64Avx_HEADER_INCLUDED)
#define hiir_PhaseHalfPi4F64Avx_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/def.h"
#include "hiir/StageDataF64Avx.h"

#include <immintrin.h>

#include <array>



namespace hiir
{



template <int NC>
class PhaseHalfPi4F64Avx
{

	static_assert ((NC > 0), "Number of coefficient must be positive.");

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef double DataType;
	static constexpr int _nbr_chn  = 4;
	static constexpr int NBR_COEFS = NC;
	static constexpr double _delay = 0;

	               PhaseHalfPi4F64Avx () noexcept;
	               PhaseHalfPi4F64Avx (const PhaseHalfPi4F64Avx <NC> &other) = default;
	               PhaseHalfPi4F64Avx (PhaseHalfPi4F64Avx <NC> &&other)      = default;
	               ~PhaseHalfPi4F64Avx ()                            = default;

	PhaseHalfPi4F64Avx <NC> &
	               operator = (const PhaseHalfPi4F64Avx <NC> &other) = default;
	PhaseHalfPi4F64Avx <NC> &
	               operator = (PhaseHalfPi4F64Avx <NC> &&other)      = default;

	void           set_coefs (const double coef_arr []) noexcept;

	hiir_FORCEINLINE void
	               process_sample (__m256d &out_0, __m256d &out_1, __m256d input) noexcept;
	void           process_block (double out_0_ptr [], double out_1_ptr [], const double in_ptr [], long nbr_spl) noexcept;

	void           clear_buffers () noexcept;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	static constexpr int _nbr_phases = 2;

	typedef std::array <StageDataF64Avx, NBR_COEFS + 2> Filter;   // Stages 0 and 1 contain only input memories

	typedef	std::array <Filter, _nbr_phases>	FilterBiPhase;

	FilterBiPhase  _bifilter;
	alignas (32) double
	               _prev [_nbr_chn];
	int            _phase;			// 0 or 1



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	bool           operator == (const PhaseHalfPi4F64Avx &other) const  = delete;
	bool           operator != (const PhaseHalfPi4F64Avx &other) const  = delete;

}; // class PhaseHalfPi4F64Avx



}  // namespace hiir



#include "hiir/PhaseHalfPi4F64Avx.hpp"



#endif   // hiir_PhaseHalfPi4F64Avx_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
