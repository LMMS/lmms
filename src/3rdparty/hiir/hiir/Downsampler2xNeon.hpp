/*****************************************************************************

        Downsampler2xNeon.hpp
        Author: Laurent de Soras, 2020

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (hiir_Downsampler2xNeon_CODEHEADER_INCLUDED)
#define hiir_Downsampler2xNeon_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/fnc_neon.h"
#include "hiir/StageProcNeonV2.h"

#include <arm_neon.h>

#include <cassert>



namespace hiir
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int NC>
constexpr int 	Downsampler2xNeon <NC>::_nbr_chn;
template <int NC>
constexpr int 	Downsampler2xNeon <NC>::NBR_COEFS;
template <int NC>
constexpr double	Downsampler2xNeon <NC>::_delay;



/*
==============================================================================
Name: ctor
Throws: Nothing
==============================================================================
*/

template <int NC>
Downsampler2xNeon <NC>::Downsampler2xNeon () noexcept
:	_filter ()
{
	for (int i = 0; i < _nbr_stages + 1; ++i)
	{
		storea (_filter [i]._mem, vdup_n_f32 (0));
	}
	if ((NBR_COEFS & 1) != 0)
	{
		const int      pos = (NBR_COEFS ^ 1) & (_stage_width - 1);
		_filter [_nbr_stages]._coef [pos] = 1;
	}

	clear_buffers ();
}



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

template <int NC>
void	Downsampler2xNeon <NC>::set_coefs (const double coef_arr []) noexcept
{
	assert (coef_arr != nullptr);

	for (int i = 0; i < NBR_COEFS; ++i)
	{
		const int      stage = (i / _stage_width) + 1;
		const int      pos   = (i ^ 1) & (_stage_width - 1);
		_filter [stage]._coef [pos] = float (coef_arr [i]);
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

template <int NC>
float	Downsampler2xNeon <NC>::process_sample (const float in_ptr [2]) noexcept
{
	assert (in_ptr != nullptr);

	auto           x = load2u (in_ptr);
	StageProcNeonV2 <_nbr_stages>::process_sample_pos (x, _filter.data ());
#if 0
	const float    out = vget_lane_f32 (vpadd_f32 (x, x), 0);
#else
	const float    out = vget_lane_f32 (x, 1) + vget_lane_f32 (x, 0);
#endif

	return out * 0.5f;
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

template <int NC>
void	Downsampler2xNeon <NC>::process_block (float out_ptr [], const float in_ptr [], long nbr_spl) noexcept
{
	assert (in_ptr  != nullptr);
	assert (out_ptr != nullptr);
	assert (out_ptr <= in_ptr || out_ptr >= in_ptr + nbr_spl * 2);
	assert (nbr_spl > 0);

	const long     n4   = process_block_quad (
		out_ptr, nullptr, in_ptr, nbr_spl, store_low, bypass
	);

	for (long pos = n4; pos < nbr_spl; ++pos)
	{
		out_ptr [pos] = process_sample (in_ptr + pos * 2);
	}
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

template <int NC>
void	Downsampler2xNeon <NC>::process_sample_split (float &low, float &high, const float in_ptr [2]) noexcept
{
	assert (in_ptr != nullptr);

	auto           x  = load2u (in_ptr);
	StageProcNeonV2 <_nbr_stages>::process_sample_pos (x, _filter.data ());
	x   *= vdup_n_f32 (0.5f);
	const float    x0 = vget_lane_f32 (x, 0);
	const float    x1 = vget_lane_f32 (x, 1);
	low  = x1 + x0;
	high = x1 - x0;
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

template <int NC>
void	Downsampler2xNeon <NC>::process_block_split (float out_l_ptr [], float out_h_ptr [], const float in_ptr [], long nbr_spl) noexcept
{
	assert (in_ptr    != nullptr);
	assert (out_l_ptr != nullptr);
	assert (out_l_ptr <= in_ptr || out_l_ptr >= in_ptr + nbr_spl * 2);
	assert (out_h_ptr != nullptr);
	assert (out_h_ptr <= in_ptr || out_h_ptr >= in_ptr + nbr_spl * 2);
	assert (out_h_ptr != out_l_ptr);
	assert (nbr_spl > 0);

	const long     n4   = process_block_quad (
		out_l_ptr, out_h_ptr, in_ptr, nbr_spl, store_low, store_high
	);

	for (long pos = n4; pos < nbr_spl; ++pos)
	{
		process_sample_split (out_l_ptr [pos], out_h_ptr [pos], in_ptr + pos * 2);
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

template <int NC>
void	Downsampler2xNeon <NC>::clear_buffers () noexcept
{
	for (int i = 0; i < _nbr_stages + 1; ++i)
	{
		storea (_filter [i]._mem, vdup_n_f32 (0));
	}
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int NC>
constexpr int	Downsampler2xNeon <NC>::_stage_width;
template <int NC>
constexpr int	Downsampler2xNeon <NC>::_nbr_stages;



template <int NC>
template <typename FL, typename FH>
long	Downsampler2xNeon <NC>::process_block_quad (float out_l_ptr [], float out_h_ptr [], const float in_ptr [], long nbr_spl, FL fnc_l, FH fnc_h) noexcept
{
	assert (_phase == 0);

	const auto     half = vdupq_n_f32 (0.5f);

	const long     n4   = nbr_spl & ~(4-1);
	for (long pos = 0; pos < n4; pos += 4)
	{
		const auto     x_0  = load4u (in_ptr + pos * 2    );
		const auto     x_2  = load4u (in_ptr + pos * 2 + 4);

		auto           y_0  = vget_low_f32 (x_0);
		auto           y_1  = vget_high_f32 (x_0);
		auto           y_2  = vget_low_f32 (x_2);
		auto           y_3  = vget_high_f32 (x_2);
		StageProcNeonV2 <_nbr_stages>::process_sample_pos (y_0, _filter.data ());
		StageProcNeonV2 <_nbr_stages>::process_sample_pos (y_1, _filter.data ());
		StageProcNeonV2 <_nbr_stages>::process_sample_pos (y_2, _filter.data ());
		StageProcNeonV2 <_nbr_stages>::process_sample_pos (y_3, _filter.data ());

		const auto     u_02 = vcombine_f32 (y_0, y_2);
		const auto     u_13 = vcombine_f32 (y_1, y_3);
		const auto     both = vtrnq_f32 (u_02, u_13);
		const auto     odd  = both.val [0];
		const auto     even = both.val [1];
		fnc_l (out_l_ptr + pos, even, odd, half);
		fnc_h (out_h_ptr + pos, even, odd, half);
	}

	return n4;
}



template <int NC>
void	Downsampler2xNeon <NC>::store_low (float *ptr, float32x4_t even, float32x4_t odd, float32x4_t half) noexcept
{
	const auto     low  = (even + odd) * half;
	storeu (ptr, low);
}



template <int NC>
void	Downsampler2xNeon <NC>::store_high (float *ptr, float32x4_t even, float32x4_t odd, float32x4_t half) noexcept
{
	const auto     high = (even - odd) * half;
	storeu (ptr, high);
}



}  // namespace hiir



#endif   // hiir_Downsampler2xNeon_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
