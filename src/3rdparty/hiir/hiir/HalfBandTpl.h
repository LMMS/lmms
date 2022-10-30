/*****************************************************************************

        HalfBandTpl.h
        Author: Laurent de Soras, 2021

Half-band filter, low-pass or high-pass.

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
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (hiir_HalfBandTpl_HEADER_INCLUDED)
#define hiir_HalfBandTpl_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/def.h"
#include "hiir/StageDataTpl.h"

#include <array>



namespace hiir
{



template <int NC, typename DT, int NCHN>
class HalfBandTpl
{

	static_assert ((NC > 0), "Number of coefficient must be positive.");
	static_assert ((NCHN > 0), "Number of channels must be positive.");

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef DT DataType;
	static constexpr int _nbr_chn  = NCHN;
	static constexpr int NBR_COEFS = NC;
	static constexpr double _delay = 0;

	void           set_coefs (const double coef_arr []) noexcept;

	hiir_FORCEINLINE DataType
	               process_sample (DataType in) noexcept;
	void           process_block (DataType out_ptr [], const DataType in_ptr [], long nbr_spl) noexcept;

	hiir_FORCEINLINE DataType
	               process_sample_hpf (DataType in) noexcept;
	void           process_block_hpf (DataType out_ptr [], const DataType in_ptr [], long nbr_spl) noexcept;

	hiir_FORCEINLINE void
	               process_sample_split (DataType &low, DataType &high, DataType in) noexcept;
	void           process_block_split (DataType out_l_ptr [], DataType out_h_ptr [], const DataType in_ptr [], long nbr_spl) noexcept;

	void           clear_buffers () noexcept;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	static constexpr int _nbr_phases = 2;

	// Stages 0 and 1 contain only input memories
	typedef std::array <StageDataTpl <DataType>, NBR_COEFS + 2> Filter;

	typedef	std::array <Filter, _nbr_phases>	FilterBiPhase;

	hiir_FORCEINLINE std::array <DataType, 2>
	               process_2_paths (DataType input) noexcept;

	FilterBiPhase  _bifilter;
	DataType       _prev  { 0.f };
	int            _phase { 0 };        // 0 or 1



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	bool           operator == (const HalfBandTpl &other) const = delete;
	bool           operator != (const HalfBandTpl &other) const = delete;

}; // class HalfBandTpl



}  // namespace hiir



#include "hiir/HalfBandTpl.hpp"



#endif   // hiir_HalfBandTpl_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
