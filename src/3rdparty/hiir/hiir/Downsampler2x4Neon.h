/*****************************************************************************

        Downsampler2x4Neon.h
        Author: Laurent de Soras, 2016

Downsamples vectors of 4 float by a factor 2 the input signal, using NEON
instruction set.

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
#if ! defined (hiir_Downsampler2x4Neon_HEADER_INCLUDED)
#define hiir_Downsampler2x4Neon_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/def.h"
#include "hiir/StageDataNeonV4.h"

#include <arm_neon.h>

#include <array>



namespace hiir
{



template <int NC>
class Downsampler2x4Neon
{

	static_assert ((NC > 0), "Number of coefficient must be positive.");

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef float DataType;
	static constexpr int _nbr_chn  = 4;
	static constexpr int NBR_COEFS = NC;
	static constexpr double _delay = -1;

	               Downsampler2x4Neon () noexcept;
	               Downsampler2x4Neon (const Downsampler2x4Neon <NC> &other) = default;
	               Downsampler2x4Neon (Downsampler2x4Neon <NC> &&other) = default;
	               ~Downsampler2x4Neon ()                            = default;

	Downsampler2x4Neon <NC> &
	               operator = (const Downsampler2x4Neon <NC> &other) = default;
	Downsampler2x4Neon <NC> &
	               operator = (Downsampler2x4Neon <NC> &&other)      = default;

	void           set_coefs (const double coef_arr []) noexcept;

	hiir_FORCEINLINE float32x4_t
	               process_sample (const float in_ptr [8]) noexcept;
	hiir_FORCEINLINE float32x4_t
	               process_sample (float32x4_t in_0, float32x4_t in_1) noexcept;
	void           process_block (float out_ptr [], const float in_ptr [], long nbr_spl) noexcept;

	hiir_FORCEINLINE void
	               process_sample_split (float32x4_t &low, float32x4_t &high, const float in_ptr [8]) noexcept;
	hiir_FORCEINLINE void
	               process_sample_split (float32x4_t &low, float32x4_t &high, float32x4_t in_0, float32x4_t in_1) noexcept;
	void           process_block_split (float out_l_ptr [], float out_h_ptr [], const float in_ptr [], long nbr_spl) noexcept;

	void           clear_buffers () noexcept;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	typedef std::array <StageDataNeonV4, NBR_COEFS + 2> Filter;   // Stages 0 and 1 contain only input memories

	Filter         _filter; // Should be the first member (thus easier to align)



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	bool           operator == (const Downsampler2x4Neon <NC> &other) const = delete;
	bool           operator != (const Downsampler2x4Neon <NC> &other) const = delete;

}; // class Downsampler2x4Neon



}  // namespace hiir



#include "hiir/Downsampler2x4Neon.hpp"



#endif   // hiir_Downsampler2x4Neon_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
