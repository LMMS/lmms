/*****************************************************************************

        Upsampler2xNeon.h
        Author: Laurent de Soras, 2020

Upsamples by a factor 2 the input signal, using NEON instruction set.

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
#if ! defined (hiir_Upsampler2xNeon_HEADER_INCLUDED)
#define hiir_Upsampler2xNeon_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/StageDataNeonV2.h"

#include <array>



namespace hiir
{



template <int NC>
class Upsampler2xNeon
{

	static_assert ((NC > 0), "Number of coefficient must be positive.");

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef float DataType;
	static constexpr int _nbr_chn  = 1;
	static constexpr int NBR_COEFS = NC;
	static constexpr double _delay = 0;

	               Upsampler2xNeon () noexcept;
	               Upsampler2xNeon (const Upsampler2xNeon <NC> &other) = default;
	               Upsampler2xNeon (Upsampler2xNeon <NC> &&other)      = default;
	               ~Upsampler2xNeon ()                                 = default;

	Upsampler2xNeon <NC> &
	               operator = (const Upsampler2xNeon <NC> &other)      = default;
	Upsampler2xNeon <NC> &
	               operator = (Upsampler2xNeon <NC> &&other)           = default;

	void           set_coefs (const double coef_arr [NBR_COEFS]) noexcept;
	inline void    process_sample (float &out_0, float &out_1, float input) noexcept;
	void           process_block (float out_ptr [], const float in_ptr [], long nbr_spl) noexcept;
	void           clear_buffers () noexcept;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	static constexpr int _stage_width = 2;
	static constexpr int _nbr_stages  =
		(NBR_COEFS + _stage_width - 1) / _stage_width;

	// Stage 0 contains only input memory
	typedef std::array <StageDataNeonV2, _nbr_stages + 1> Filter;

	hiir_FORCEINLINE long
	               process_block_quad (float out_ptr [], const float in_ptr [], long nbr_spl) noexcept;

	Filter         _filter;		// Should be the first member (thus easier to align)



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	bool           operator == (const Upsampler2xNeon <NC> &other) const = delete;
	bool           operator != (const Upsampler2xNeon <NC> &other) const = delete;

}; // class Upsampler2xNeon



}  // namespace hiir



#include "hiir/Upsampler2xNeon.hpp"



#endif   // hiir_Upsampler2xNeon_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
