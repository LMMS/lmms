/*****************************************************************************

        Downsampler2xTpl.h
        Author: Laurent de Soras, 2005

Downsamples by a factor 2 the input signal, using FPU.

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



#if ! defined (hiir_Downsampler2xTpl_HEADER_INCLUDED)
#define hiir_Downsampler2xTpl_HEADER_INCLUDED

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
class Downsampler2xTpl
{

	static_assert ((NC > 0), "Number of coefficient must be positive.");
	static_assert ((NCHN > 0), "Number of channels must be positive.");

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef DT DataType;
	static constexpr int _nbr_chn  = NCHN;
	static constexpr int NBR_COEFS = NC;
	static constexpr double _delay = -1;

	void           set_coefs (const double coef_arr []) noexcept;

	hiir_FORCEINLINE DataType
	               process_sample (const DataType in_ptr [2]) noexcept;
	void           process_block (DataType out_ptr [], const DataType in_ptr [], long nbr_spl) noexcept;

	hiir_FORCEINLINE void
	               process_sample_split (DataType &low, DataType &high, const DataType in_ptr [2]) noexcept;
	void           process_block_split (DataType out_l_ptr [], DataType out_h_ptr [], const DataType in_ptr [], long nbr_spl) noexcept;

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

	bool           operator == (const Downsampler2xTpl <NC, DT, NCHN> &other) = delete;
	bool           operator != (const Downsampler2xTpl <NC, DT, NCHN> &other) = delete;

}; // class Downsampler2xTpl



}  // namespace hiir



#include "hiir/Downsampler2xTpl.hpp"



#endif   // hiir_Downsampler2xTpl_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
