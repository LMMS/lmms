/*****************************************************************************

        Upsampler2xTpl.h
        Author: Laurent de Soras, 2005

Upsamples by a factor 2 the input signal, using FPU.

Template parameters:

- NC: number of coefficients, > 0

- DT: data type. Requires:
	DT::DT ();
	DT::DT (float);
	DT::DT (int);
	DT::DT (const DT &)
	DT & DT::operator = (const DT &);
	DT & DT::operator += (const DT &);
	DT & DT::operator -= (const DT &);
	DT & DT::operator *= (const DT &);
	DT operator + (DT, const DT &);
	DT operator - (DT, const DT &);
	DT operator * (DT, const DT &);

- NCHN: number of contained scalars, if DT is a vector type.

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (hiir_Upsampler2xTpl_HEADER_INCLUDED)
#define hiir_Upsampler2xTpl_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250) // "Inherits via dominance."
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/def.h"
#include "hiir/StageDataTpl.h"

#include <array>



namespace hiir
{



template <int NC, typename DT, int NCHN>
class Upsampler2xTpl
{

	static_assert ((NC > 0), "Number of coefficient must be positive.");
	static_assert ((NCHN > 0), "Number of channels must be positive.");

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef DT DataType;
	static constexpr int _nbr_chn  = NCHN;
	static constexpr int NBR_COEFS = NC;
	static constexpr double _delay = 0;

	void           set_coefs (const double coef_arr [NBR_COEFS]) noexcept;
	hiir_FORCEINLINE void
	               process_sample (DataType &out_0, DataType &out_1, DataType input) noexcept;
	void           process_block (DataType out_ptr [], const DataType in_ptr [], long nbr_spl) noexcept;
	void           clear_buffers () noexcept;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	// Stages 0 and 1 contain only input memories
	typedef std::array <StageDataTpl <DataType>, NBR_COEFS + 2> Filter;

	Filter         _filter;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	bool           operator == (const Upsampler2xTpl <NC, DT, NCHN> &other) = delete;
	bool           operator != (const Upsampler2xTpl <NC, DT, NCHN> &other) = delete;

}; // class Upsampler2xTpl



}  // namespace hiir



#include "hiir/Upsampler2xTpl.hpp"



#endif   // hiir_Upsampler2xTpl_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
