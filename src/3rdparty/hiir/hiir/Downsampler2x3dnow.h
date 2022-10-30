/*****************************************************************************

        Downsampler2x3dnow.h
        Author: Laurent de Soras, 2005

Downsamples by a factor 2 the input signal, using 3DNow! instruction set.

Template parameters:
	- NC: number of coefficients, > 0

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (hiir_Downsampler2x3dnow_HEADER_INCLUDED)
#define hiir_Downsampler2x3dnow_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250) // "Inherits via dominance."
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/def.h"
#include "hiir/StageData3dnow.h"

#include <array>



namespace hiir
{



template <int NC>
class Downsampler2x3dnow
{

	static_assert ((NC > 0), "Number of coefficient must be positive.");

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef float DataType;
	static constexpr int _nbr_chn  = 1;
	static constexpr int NBR_COEFS = NC;
	static constexpr double _delay = -1;

	               Downsampler2x3dnow () noexcept;
	               Downsampler2x3dnow (const Downsampler2x3dnow <NC> &other) = default;
	               Downsampler2x3dnow (Downsampler2x3dnow <NC> &&other)      = default;
	               ~Downsampler2x3dnow ()                            = default;

	Downsampler2x3dnow <NC> &
	               operator = (const Downsampler2x3dnow <NC> &other) = default;
	Downsampler2x3dnow <NC> &
	               operator = (Downsampler2x3dnow <NC> &&other)      = default;

	void           set_coefs (const double coef_arr []) noexcept;

	hiir_FORCEINLINE float
	               process_sample (const float in_ptr [2]) noexcept;
	void           process_block (float out_ptr [], const float in_ptr [], long nbr_spl) noexcept;

	hiir_FORCEINLINE void
	               process_sample_split (float &low, float &high, const float in_ptr [2]) noexcept;
	void           process_block_split (float out_l_ptr [], float out_h_ptr [], const float in_ptr [], long nbr_spl) noexcept;

	void           clear_buffers () noexcept;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	static constexpr int  _stage_width = 2;
	static constexpr int  _nbr_stages  =
		(NBR_COEFS + _stage_width - 1) / _stage_width;

	typedef	std::array <StageData3dnow, _nbr_stages + 1>	Filter;	// Stage 0 contains only input memory

	Filter         _filter;		// Should be the first member (thus easier to align)



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	bool           operator == (const Downsampler2x3dnow <NC> &other) = delete;
	bool           operator != (const Downsampler2x3dnow <NC> &other) = delete;

}; // class Downsampler2x3dnow



}  // namespace hiir



#include "hiir/Downsampler2x3dnow.hpp"



#endif   // hiir_Downsampler2x3dnow_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
