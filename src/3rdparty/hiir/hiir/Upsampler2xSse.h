/*****************************************************************************

        Upsampler2xSse.h
        Author: Laurent de Soras, 2020

Upsamples by a factor 2 the input signal, using SSE instruction set.

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
#if ! defined (hiir_Upsampler2xSse_HEADER_INCLUDED)
#define hiir_Upsampler2xSse_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/def.h"
#include "hiir/StageDataSse.h"

#include <xmmintrin.h>

#include <array>



namespace hiir
{



template <int NC>
class Upsampler2xSse
{

	static_assert ((NC > 0), "Number of coefficient must be positive.");

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef float DataType;
	static constexpr int _nbr_chn  = 1;
	static constexpr int NBR_COEFS = NC;
	static constexpr double _delay = 0;

	               Upsampler2xSse () noexcept;
	               Upsampler2xSse (const Upsampler2xSse &other)    = default;
	               Upsampler2xSse (Upsampler2xSse &&other)         = default;
	               ~Upsampler2xSse ()                              = default;

	Upsampler2xSse &
	               operator = (const Upsampler2xSse &other)        = default;
	Upsampler2xSse &
	               operator = (Upsampler2xSse &&other)             = default;

	void           set_coefs (const double coef_arr [NBR_COEFS]) noexcept;

	hiir_FORCEINLINE void
	               process_sample (float &out_0, float &out_1, float input) noexcept;
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
	typedef std::array <StageDataSse, _nbr_stages + 1> Filter;

	Filter         _filter;		// Should be the first member (thus easier to align)



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	bool           operator == (const Upsampler2xSse &other) const = delete;
	bool           operator != (const Upsampler2xSse &other) const = delete;

}; // class Upsampler2xSse



}  // namespace hiir



#include "hiir/Upsampler2xSse.hpp"



#endif   // hiir_Upsampler2xSse_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
