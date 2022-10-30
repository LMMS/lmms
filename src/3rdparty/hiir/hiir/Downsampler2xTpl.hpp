/*****************************************************************************

        Downsampler2xTpl.hpp
        Author: Laurent de Soras, 2005

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if defined (hiir_Downsampler2xTpl_CURRENT_CODEHEADER)
	#error Recursive inclusion of Downsampler2xTpl code header.
#endif
#define hiir_Downsampler2xTpl_CURRENT_CODEHEADER

#if ! defined (hiir_Downsampler2xTpl_CODEHEADER_INCLUDED)
#define hiir_Downsampler2xTpl_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/StageProcTpl.h"

#include <cassert>



namespace hiir
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int NC, typename DT, int NCHN>
constexpr int 	Downsampler2xTpl <NC, DT, NCHN>::_nbr_chn;
template <int NC, typename DT, int NCHN>
constexpr int 	Downsampler2xTpl <NC, DT, NCHN>::NBR_COEFS;
template <int NC, typename DT, int NCHN>
constexpr double	Downsampler2xTpl <NC, DT, NCHN>::_delay;



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
void	Downsampler2xTpl <NC, DT, NCHN>::set_coefs (const double coef_arr []) noexcept
{
	assert (coef_arr != nullptr);

	for (int i = 0; i < NBR_COEFS; ++i)
	{
		_filter [i + 2]._coef = DataType (coef_arr [i]);
	}
}



/*
==============================================================================
Name: process_sample
Description:
	Downsamples (x2) one pair of samples, to generate one output sample.
Input parameters:
	- in_ptr: pointer on the two samples to decimate
Returns: Samplerate-reduced sample.
Throws: Nothing
==============================================================================
*/

template <int NC, typename DT, int NCHN>
typename Downsampler2xTpl <NC, DT, NCHN>::DataType	Downsampler2xTpl <NC, DT, NCHN>::process_sample (const DataType in_ptr [2]) noexcept
{
	assert (in_ptr != nullptr);

	DataType       spl_0 (in_ptr [1]);
	DataType       spl_1 (in_ptr [0]);

	StageProcTpl <NBR_COEFS, DataType>::process_sample_pos (
		NBR_COEFS, spl_0, spl_1, _filter.data ()
	);

	return DataType (0.5f) * (spl_0 + spl_1);
}



/*
==============================================================================
Name: process_block
Description:
	Downsamples (x2) a block of samples.
	Input and output blocks may overlap, see assert() for details.
Input parameters:
	- in_ptr: Input array, containing nbr_spl * 2 samples.
	- nbr_spl: Number of samples to output, > 0
Output parameters:
	- out_ptr: Array for the output samples, capacity: nbr_spl samples.
Throws: Nothing
==============================================================================
*/

template <int NC, typename DT, int NCHN>
void	Downsampler2xTpl <NC, DT, NCHN>::process_block (DataType out_ptr [], const DataType in_ptr [], long nbr_spl) noexcept
{
	assert (in_ptr != nullptr);
	assert (out_ptr != nullptr);
	assert (out_ptr <= in_ptr || out_ptr >= in_ptr + nbr_spl * 2);
	assert (nbr_spl > 0);

	long           pos = 0;
	do
	{
		out_ptr [pos] = process_sample (&in_ptr [pos * 2]);
		++pos;
	}
	while (pos < nbr_spl);
}



/*
==============================================================================
Name: process_sample_split
Description:
	Split (spectrum-wise) in half a pair of samples. The lower part of the
	spectrum is a classic downsampling, equivalent to the output of
	process_sample().
	The higher part is the complementary signal: original filter response
	is flipped from left to right, becoming a high-pass filter with the same
	cutoff frequency. This signal is then critically sampled (decimation by 2),
	flipping the spectrum: Fs/4...Fs/2 becomes Fs/4...0.
Input parameters:
	- in_ptr: pointer on the pair of input samples
Output parameters:
	- low: output sample, lower part of the spectrum (downsampling)
	- high: output sample, higher part of the spectrum.
Throws: Nothing
==============================================================================
*/

template <int NC, typename DT, int NCHN>
void	Downsampler2xTpl <NC, DT, NCHN>::process_sample_split (DataType &low, DataType &high, const DataType in_ptr [2]) noexcept
{
	assert (in_ptr != nullptr);

	DataType       spl_0 = in_ptr [1];
	DataType       spl_1 = in_ptr [0];

	StageProcTpl <NBR_COEFS, DataType>::process_sample_pos (
		NBR_COEFS, spl_0, spl_1, _filter.data ()
	);

	low  = (spl_0 + spl_1) * DataType (0.5f);
	high =  spl_0 - low; // (spl_0 - spl_1) * DataType (0.5f);
}



/*
==============================================================================
Name: process_block_split
Description:
	Split (spectrum-wise) in half a block of samples. The lower part of the
	spectrum is a classic downsampling, equivalent to the output of
	process_block().
	The higher part is the complementary signal: original filter response
	is flipped from left to right, becoming a high-pass filter with the same
	cutoff frequency. This signal is then critically sampled (decimation by 2),
	flipping the spectrum: Fs/4...Fs/2 becomes Fs/4...0.
	Input and output blocks may overlap, see assert() for details.
Input parameters:
	- in_ptr: Input array, containing nbr_spl * 2 samples.
	- nbr_spl: Number of samples for each output, > 0
Output parameters:
	- out_l_ptr: Array for the output samples, lower part of the spectrum
		(downsampling). Capacity: nbr_spl samples.
	- out_h_ptr: Array for the output samples, higher part of the spectrum.
		Capacity: nbr_spl samples.
Throws: Nothing
==============================================================================
*/

template <int NC, typename DT, int NCHN>
void	Downsampler2xTpl <NC, DT, NCHN>::process_block_split (DataType out_l_ptr [], DataType out_h_ptr [], const DataType in_ptr [], long nbr_spl) noexcept
{
	assert (in_ptr    != nullptr);
	assert (out_l_ptr != nullptr);
	assert (out_l_ptr <= in_ptr || out_l_ptr >= in_ptr + nbr_spl * 2);
	assert (out_h_ptr != nullptr);
	assert (out_h_ptr <= in_ptr || out_h_ptr >= in_ptr + nbr_spl * 2);
	assert (out_h_ptr != out_l_ptr);
	assert (nbr_spl > 0);

	long           pos = 0;
	do
	{
		process_sample_split (
			out_l_ptr [pos],
			out_h_ptr [pos],
			&in_ptr [pos * 2]
		);
		++pos;
	}
	while (pos < nbr_spl);
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
void	Downsampler2xTpl <NC, DT, NCHN>::clear_buffers () noexcept
{
	for (int i = 0; i < NBR_COEFS + 2; ++i)
	{
		_filter [i]._mem = DataType (0.f);
	}
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}  // namespace hiir



#endif   // hiir_Downsampler2xTpl_CODEHEADER_INCLUDED

#undef hiir_Downsampler2xTpl_CURRENT_CODEHEADER



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
