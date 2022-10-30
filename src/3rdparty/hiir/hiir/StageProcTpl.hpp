/*****************************************************************************

        StageProcTpl.hpp
        Author: Laurent de Soras, 2005

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if defined (hiir_StageProc_CURRENT_CODEHEADER)
	#error Recursive inclusion of StageProcTpl code header.
#endif
#define hiir_StageProc_CURRENT_CODEHEADER

#if ! defined (hiir_StageProc_CODEHEADER_INCLUDED)
#define hiir_StageProc_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



namespace hiir
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int REMAINING, typename DT>
void	StageProcTpl <REMAINING, DT>::process_sample_pos (const int nbr_coefs, DT &spl_0, DT &spl_1, StageDataTpl <DT> *stage_arr) noexcept
{
	const int      cnt   = nbr_coefs + 2 - REMAINING;

	DT             tmp_0 = spl_0;
	tmp_0 -= stage_arr [cnt    ]._mem;
	tmp_0 *= stage_arr [cnt    ]._coef;
	tmp_0 += stage_arr [cnt - 2]._mem;

	DT             tmp_1 = spl_1;
	tmp_1 -= stage_arr [cnt + 1]._mem;
	tmp_1 *= stage_arr [cnt + 1]._coef;
	tmp_1 += stage_arr [cnt - 1]._mem;

	stage_arr [cnt - 2]._mem = spl_0;
	stage_arr [cnt - 1]._mem = spl_1;

	spl_0 = tmp_0;
	spl_1 = tmp_1;

	StageProcTpl <REMAINING - 2, DT>::process_sample_pos (
		nbr_coefs,
		spl_0,
		spl_1,
		stage_arr
	);
}

template <typename DT>
void	StageProcTpl <1, DT>::process_sample_pos (const int nbr_coefs, DT &spl_0, DT &spl_1, StageDataTpl <DT> *stage_arr) noexcept
{
	const int      cnt   = nbr_coefs + 2 - 1;

	DT             tmp_0 = spl_0;
	tmp_0 -= stage_arr [cnt    ]._mem;
	tmp_0 *= stage_arr [cnt    ]._coef;
	tmp_0 += stage_arr [cnt - 2]._mem;

	stage_arr [cnt - 2]._mem = spl_0;
	stage_arr [cnt - 1]._mem = spl_1;
	stage_arr [cnt    ]._mem = tmp_0;

	spl_0 = tmp_0;
}

template <typename DT>
void	StageProcTpl <0, DT>::process_sample_pos (const int nbr_coefs, DT &spl_0, DT &spl_1, StageDataTpl <DT> *stage_arr) noexcept
{
	const int      cnt = nbr_coefs + 2;

	stage_arr [cnt - 2]._mem = spl_0;
	stage_arr [cnt - 1]._mem = spl_1;
}



template <int REMAINING, typename DT>
void	StageProcTpl <REMAINING, DT>::process_sample_neg (const int nbr_coefs, DT &spl_0, DT &spl_1, StageDataTpl <DT> *stage_arr) noexcept
{
	const int      cnt   = nbr_coefs + 2 - REMAINING;

	DT             tmp_0 = spl_0;
	tmp_0 += stage_arr [cnt    ]._mem;
	tmp_0 *= stage_arr [cnt    ]._coef;
	tmp_0 -= stage_arr [cnt - 2]._mem;

	DT             tmp_1 = spl_1;
	tmp_1 += stage_arr [cnt + 1]._mem;
	tmp_1 *= stage_arr [cnt + 1]._coef;
	tmp_1 -= stage_arr [cnt - 1]._mem;

	stage_arr [cnt - 2]._mem = spl_0;
	stage_arr [cnt - 1]._mem = spl_1;

	spl_0 = tmp_0;
	spl_1 = tmp_1;

	StageProcTpl <REMAINING - 2, DT>::process_sample_neg (
		nbr_coefs,
		spl_0,
		spl_1,
		stage_arr
	);
}

template <typename DT>
void	StageProcTpl <1, DT>::process_sample_neg (const int nbr_coefs, DT &spl_0, DT &spl_1, StageDataTpl <DT> *stage_arr) noexcept
{
	const int      cnt   = nbr_coefs + 2 - 1;

	DT             tmp_0 = spl_0;
	tmp_0 += stage_arr [cnt    ]._mem;
	tmp_0 *= stage_arr [cnt    ]._coef;
	tmp_0 -= stage_arr [cnt - 2]._mem;

	stage_arr [cnt - 2]._mem = spl_0;
	stage_arr [cnt - 1]._mem = spl_1;
	stage_arr [cnt    ]._mem = tmp_0;

	spl_0 = tmp_0;
}

template <typename DT>
void	StageProcTpl <0, DT>::process_sample_neg (const int nbr_coefs, DT &spl_0, DT &spl_1, StageDataTpl <DT> *stage_arr) noexcept
{
	const int      cnt = nbr_coefs + 2;

	stage_arr [cnt - 2]._mem = spl_0;
	stage_arr [cnt - 1]._mem = spl_1;
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}  // namespace hiir



#endif   // hiir_StageProc_CODEHEADER_INCLUDED

#undef hiir_StageProc_CURRENT_CODEHEADER



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
