/*****************************************************************************

        PhaseHalfPi16Avx512.h
        Author: Laurent de Soras, 2020

From the input signal, generates two signals with a pi/2 phase shift, using
AVX-512 instruction set. Works on vectors of 16 float.

This object must be aligned on a 64-byte boundary!

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
#if ! defined (hiir_PhaseHalfPi16Avx512_HEADER_INCLUDED)
#define hiir_PhaseHalfPi16Avx512_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/def.h"
#include "hiir/StageDataAvx512.h"

#include <immintrin.h>

#include <array>



namespace hiir
{



template <int NC>
class PhaseHalfPi16Avx512
{

	static_assert ((NC > 0), "Number of coefficient must be positive.");

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef float DataType;
	static constexpr int _nbr_chn  = 16;
	static constexpr int NBR_COEFS = NC;
	static constexpr double _delay = 0;

	               PhaseHalfPi16Avx512 () noexcept;
	               PhaseHalfPi16Avx512 (const PhaseHalfPi16Avx512 <NC> &other) = default;
	               PhaseHalfPi16Avx512 (PhaseHalfPi16Avx512 <NC> &&other)      = default;
	               ~PhaseHalfPi16Avx512 ()                                 = default;

	PhaseHalfPi16Avx512 <NC> &
	               operator = (const PhaseHalfPi16Avx512 <NC> &other)      = default;
	PhaseHalfPi16Avx512 <NC> &
	               operator = (PhaseHalfPi16Avx512 <NC> &&other)           = default;

	void           set_coefs (const double coef_arr []) noexcept;

	hiir_FORCEINLINE void
	               process_sample (__m512 &out_0, __m512 &out_1, __m512 input) noexcept;
	void           process_block (float out_0_ptr [], float out_1_ptr [], const float in_ptr [], long nbr_spl) noexcept;

	void           clear_buffers () noexcept;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	static constexpr int _nbr_phases = 2;

	typedef std::array <StageDataAvx512, NBR_COEFS + 2> Filter;   // Stages 0 and 1 contain only input memories

	typedef	std::array <Filter, _nbr_phases>	FilterBiPhase;

	FilterBiPhase  _bifilter;
	alignas (64) float
	               _prev [_nbr_chn];
	int            _phase;			// 0 or 1



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	bool           operator == (const PhaseHalfPi16Avx512 &other) const = delete;
	bool           operator != (const PhaseHalfPi16Avx512 &other) const = delete;

}; // class PhaseHalfPi16Avx512



}  // namespace hiir



#include "hiir/PhaseHalfPi16Avx512.hpp"



#endif   // hiir_PhaseHalfPi16Avx512_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
