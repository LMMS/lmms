/*****************************************************************************

        HalfBandTpl.hpp
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#if ! defined (hiir_HalfBandTpl_CODEHEADER_INCLUDED)
#define hiir_HalfBandTpl_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/StageProcTpl.h"

#include <cassert>



namespace hiir
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int NC, typename DT, int NCHN>
constexpr int 	HalfBandTpl <NC, DT, NCHN>::_nbr_chn;
template <int NC, typename DT, int NCHN>
constexpr int 	HalfBandTpl <NC, DT, NCHN>::NBR_COEFS;
template <int NC, typename DT, int NCHN>
constexpr double	HalfBandTpl <NC, DT, NCHN>::_delay;



/*
==============================================================================
Name: set_coefs
Description:
	Sets filter coefficients. Generate them with the PolyphaseIir2Designer
	class.
	Call this function before doing any processing.
Input parameters:
	- coef_arr: Array of coefficients. There should be as many coefficients as
		mentioned in the class template parameter.
Throws: Nothing
==============================================================================
*/

template <int NC, typename DT, int NCHN>
void	HalfBandTpl <NC, DT, NCHN>::set_coefs (const double coef_arr []) noexcept
{
	assert (coef_arr != nullptr);

	for (int i = 0; i < NBR_COEFS; ++i)
	{
		const DataType c = DataType (coef_arr [i]);
		_bifilter [0] [i + 2]._coef = c;
		_bifilter [1] [i + 2]._coef = c;
	}
}



/*
==============================================================================
Name: process_sample
Description:
	Performs a half-band low-pass filtering on a sample.
Input parameters:
	- in: sample from the stream to be filtered.
Returns: filtered sample.
Throws: Nothing
==============================================================================
*/

template <int NC, typename DT, int NCHN>
typename HalfBandTpl <NC, DT, NCHN>::DataType	HalfBandTpl <NC, DT, NCHN>::process_sample (DataType in) noexcept
{
	const auto     path_arr = process_2_paths (in);
	const auto     out = (path_arr [0] + path_arr [1]) * DataType (0.5f);

	return out;
}



/*
==============================================================================
Name: process_block
Description:
	Performs a half-band low-pass filtering on a block of samples.
	Input and output blocks may overlap, see assert() for details.
Input parameters:
	- in_ptr: Input array, containing nbr_spl samples.
	- nbr_spl: Number of samples to output, > 0
Output parameters:
	- out_ptr: Array for the output samples, capacity: nbr_spl samples.
Throws: Nothing
==============================================================================
*/

template <int NC, typename DT, int NCHN>
void	HalfBandTpl <NC, DT, NCHN>::process_block (DataType out_ptr [], const DataType in_ptr [], long nbr_spl) noexcept
{
	assert (in_ptr  != nullptr);
	assert (out_ptr != nullptr);
	assert (out_ptr <= in_ptr || out_ptr >= in_ptr + nbr_spl);
	assert (nbr_spl > 0);

	for (long pos = 0; pos < nbr_spl; ++pos)
	{
		out_ptr [pos] = process_sample (in_ptr [pos]);
	}
}



/*
==============================================================================
Name: process_sample_hpf
Description:
	Performs a half-band high-pass filtering on a sample.
Input parameters:
	- in: sample from the stream to be filtered.
Returns: filtered sample.
Throws: Nothing
==============================================================================
*/

template <int NC, typename DT, int NCHN>
typename HalfBandTpl <NC, DT, NCHN>::DataType	HalfBandTpl <NC, DT, NCHN>::process_sample_hpf (DataType in) noexcept
{
	const auto     path_arr = process_2_paths (in);
	const auto     out = (path_arr [0] - path_arr [1]) * DataType (0.5f);

	return out;
}



/*
==============================================================================
Name: process_block_hpf
Description:
	Performs a half-band high-pass filtering on a block of samples.
	Input and output blocks may overlap, see assert() for details.
Input parameters:
	- in_ptr: Input array, containing nbr_spl samples.
	- nbr_spl: Number of samples to output, > 0
Output parameters:
	- out_ptr: Array for the output samples, capacity: nbr_spl samples.
Throws: Nothing
==============================================================================
*/

template <int NC, typename DT, int NCHN>
void	HalfBandTpl <NC, DT, NCHN>::process_block_hpf (DataType out_ptr [], const DataType in_ptr [], long nbr_spl) noexcept
{
	assert (in_ptr  != nullptr);
	assert (out_ptr != nullptr);
	assert (out_ptr <= in_ptr || out_ptr >= in_ptr + nbr_spl);
	assert (nbr_spl > 0);

	for (long pos = 0; pos < nbr_spl; ++pos)
	{
		out_ptr [pos] = process_sample_hpf (in_ptr [pos]);
	}
}



/*
==============================================================================
Name: process_sample_split
Description:
	Splits (spectrum-wise) in half a sample from a stream. Both part are
	results of a low-pass and a high-pass filtering, equivalent to the output
	of process_sample() and process_sample_hpf().
Input parameters:
	- in: sample from the stream to be filtered.
Output parameters:
	- low: output sample, lower part of the spectrum.
	- high: output sample, higher part of the spectrum.
Throws: Nothing
==============================================================================
*/

template <int NC, typename DT, int NCHN>
void	HalfBandTpl <NC, DT, NCHN>::process_sample_split (DataType &low, DataType &high, DataType in) noexcept
{
	const auto     path_arr = process_2_paths (in);
	const auto     half     = DataType (0.5f);
	low  = (path_arr [0] + path_arr [1]) * half;
	high = (path_arr [0] - path_arr [1]) * half;
}



/*
==============================================================================
Name: process_block_split
Description:
	Splits (spectrum-wise) in half a block of samples. Both part are
	results of a low-pass and a high-pass filtering, equivalent to the output
	of process_sample() and process_sample_hpf().
	Input and output blocks may overlap, see assert() for details.
Input parameters:
	- in_ptr: Input array, containing nbr_spl samples.
	- nbr_spl: Number of samples to output, > 0
Output parameters:
	- out_l_ptr: Array for the output samples, lower part of the spectrum
		Capacity: nbr_spl samples.
	- out_h_ptr: Array for the output samples, higher part of the spectrum.
		Capacity: nbr_spl samples.
Throws: Nothing
==============================================================================
*/

template <int NC, typename DT, int NCHN>
void	HalfBandTpl <NC, DT, NCHN>::process_block_split (DataType out_l_ptr [], DataType out_h_ptr [], const DataType in_ptr [], long nbr_spl) noexcept
{
	assert (in_ptr    != nullptr);
	assert (out_l_ptr != nullptr);
	assert (out_h_ptr != nullptr);
	assert (out_l_ptr <= in_ptr || out_l_ptr >= in_ptr + nbr_spl);
	assert (out_h_ptr <= in_ptr || out_h_ptr >= in_ptr + nbr_spl);
	assert (out_l_ptr + nbr_spl <= out_h_ptr || out_h_ptr + nbr_spl <= out_l_ptr);
	assert (nbr_spl > 0);

	for (long pos = 0; pos < nbr_spl; ++pos)
	{
		process_sample_split (out_l_ptr [pos], out_h_ptr [pos], in_ptr [pos]);
	}
}



/*
==============================================================================
Name: clear_buffers
Description:
	Clears filter memory, as if it processed silence since an infinite amount
	of time.
Throws: Nothing
==============================================================================
*/

template <int NC, typename DT, int NCHN>
void	HalfBandTpl <NC, DT, NCHN>::clear_buffers () noexcept
{
	for (int i = 0; i < NBR_COEFS + 2; ++i)
	{
		_bifilter [0] [i]._mem = DataType (0.f);
		_bifilter [1] [i]._mem = DataType (0.f);
	}

	_prev  = DataType (0.f);
	_phase = 0;
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int NC, typename DT, int NCHN>
constexpr int 	HalfBandTpl <NC, DT, NCHN>::_nbr_phases;



// Shared processing function, outputs both paths of the all-pass filter pair.
template <int NC, typename DT, int NCHN>
std::array <typename HalfBandTpl <NC, DT, NCHN>::DataType, 2>	HalfBandTpl <NC, DT, NCHN>::process_2_paths (DataType input) noexcept
{
	std::array <DataType, 2>   path_arr { input, _prev };

	StageProcTpl <NBR_COEFS, DataType>::process_sample_pos (
		NBR_COEFS, path_arr [0], path_arr [1], _bifilter [_phase].data ()
	);

	_prev  = input;
	_phase = 1 - _phase;

	return path_arr;
}



}  // namespace hiir



#endif   // hiir_HalfBandTpl_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
