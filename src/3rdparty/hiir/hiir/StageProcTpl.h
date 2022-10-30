/*****************************************************************************

        StageProcTpl.h
        Author: Laurent de Soras, 2005

Template parameters:

- REMAINING: Number of remaining coefficients to process, >= 0

- DT: Data type (float or double)



--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (hiir_StageProc_HEADER_INCLUDED)
#define hiir_StageProc_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250) // "Inherits via dominance."
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/def.h"
#include "hiir/StageDataTpl.h"



namespace hiir
{



template <int REMAINING, typename DT>
class StageProcTpl
{

	static_assert ((REMAINING >= 0), "REMAINING must be >= 0");

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	static hiir_FORCEINLINE void
	               process_sample_pos (const int nbr_coefs, DT &spl_0, DT &spl_1, StageDataTpl <DT> *stage_arr) noexcept;
	static hiir_FORCEINLINE void
	               process_sample_neg (const int nbr_coefs, DT &spl_0, DT &spl_1, StageDataTpl <DT> *stage_arr) noexcept;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               StageProcTpl ()                                          = delete;
	               StageProcTpl (const StageProcTpl <REMAINING, DT> &other) = delete;
	               StageProcTpl (StageProcTpl <REMAINING, DT> &&other)      = delete;
	               ~StageProcTpl ()                                         = delete;
	StageProcTpl <REMAINING, DT> &
	               operator = (const StageProcTpl <REMAINING, DT> &other)   = delete;
	StageProcTpl <REMAINING, DT> &
	               operator = (StageProcTpl <REMAINING, DT> &&other)        = delete;
	bool           operator == (const StageProcTpl <REMAINING, DT> &other)  = delete;
	bool           operator != (const StageProcTpl <REMAINING, DT> &other)  = delete;

}; // class StageProcTpl

template <typename DT>
class StageProcTpl <0, DT>
{

public:

	static hiir_FORCEINLINE void
	               process_sample_pos (const int nbr_coefs, DT &spl_0, DT &spl_1, StageDataTpl <DT> *stage_arr) noexcept;
	static hiir_FORCEINLINE void
	               process_sample_neg (const int nbr_coefs, DT &spl_0, DT &spl_1, StageDataTpl <DT> *stage_arr) noexcept;

private:

	               StageProcTpl ()                                  = delete;
	               StageProcTpl (const StageProcTpl <0, DT> &other) = delete;
	               StageProcTpl (StageProcTpl <0, DT> &&other)      = delete;
	               ~StageProcTpl ()                                 = delete;
	StageProcTpl <0, DT> &
	               operator = (const StageProcTpl <0, DT> &other)   = delete;
	StageProcTpl <0, DT> &
	               operator = (StageProcTpl <0, DT> &&other)        = delete;
	bool           operator == (const StageProcTpl <0, DT> &other)  = delete;
	bool           operator != (const StageProcTpl <0, DT> &other)  = delete;


}; // class StageProcTpl

template <typename DT>
class StageProcTpl <1, DT>
{

public:

	static hiir_FORCEINLINE void
	               process_sample_pos (const int nbr_coefs, DT &spl_0, DT &spl_1, StageDataTpl <DT> *stage_arr) noexcept;
	static hiir_FORCEINLINE void
	               process_sample_neg (const int nbr_coefs, DT &spl_0, DT &spl_1, StageDataTpl <DT> *stage_arr) noexcept;

private:

	               StageProcTpl ()                                  = delete;
	               StageProcTpl (const StageProcTpl <1, DT> &other) = delete;
	               StageProcTpl (StageProcTpl <1, DT> &&other)      = delete;
	               ~StageProcTpl ()                                 = delete;
	StageProcTpl <1, DT> &
	               operator = (const StageProcTpl <1, DT> &other)   = delete;
	StageProcTpl <1, DT> &
	               operator = (StageProcTpl <1, DT> &&other)        = delete;
	bool           operator == (const StageProcTpl <1, DT> &other)  = delete;
	bool           operator != (const StageProcTpl <1, DT> &other)  = delete;


}; // class StageProcTpl



}  // namespace hiir



#include "hiir/StageProcTpl.hpp"



#endif   // hiir_StageProc_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
